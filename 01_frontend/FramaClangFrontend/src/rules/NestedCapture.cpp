// SPDX-License-Identifier: Apache-2.0
#include "rules/NestedCapture.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <tuple>

using namespace clang;
namespace buddy::framac {
namespace {
/// @param L 当前 lambda；AST 的父节点可能形成图，不能假定只有一条父链。
/// @param AST 当前翻译单元，只借用父节点信息。
/// @return 是否存在包围它的 lambda；普通函数/局部类方法是独立作用域边界。
bool hasEnclosingLambda(LambdaExpr *L, ASTContext &AST) {
  std::vector<DynTypedNode> Pending{DynTypedNode::create(*L)};
  std::set<const void *> Seen;
  while (!Pending.empty()) {
    auto Node = Pending.back();
    Pending.pop_back();
    for (const auto &Parent : AST.getParents(Node)) {
      if (Parent.get<LambdaExpr>())
        return true;
      if (const auto *F = Parent.get<FunctionDecl>()) {
        const auto *M = dyn_cast<CXXMethodDecl>(F);
        if (!M || !M->getParent()->isLambda())
          continue;
      }
      const void *Key = Parent.getMemoizationData();
      if (Key && Seen.insert(Key).second)
        Pending.push_back(Parent);
    }
  }
  return false;
}

/// 只收集直属 lambda。遇到子闭包暂不向下，由 buildTree 分别进入其函数体。
/// 普通局部类的方法并非当前捕获链，本轮保守标记为需人工审查。
class DirectLambdas : public RecursiveASTVisitor<DirectLambdas> {
public:
  std::vector<LambdaExpr *> Children;
  bool LocalClass = false;
  bool Unevaluated = false;
  bool VisitCXXNoexceptExpr(CXXNoexceptExpr *) {
    Unevaluated = true;
    return true;
  }
  bool TraverseLambdaExpr(LambdaExpr *L) {
    Children.push_back(L);
    return true;
  }
  bool TraverseCXXRecordDecl(CXXRecordDecl *D) {
    if (!D->isLambda())
      LocalClass = true;
    return true;
  }
};

/// 所有节点仅在此次 AST 回调期间借用；拥有的是树结构，不是 AST 节点。
struct CaptureNode {
  LambdaExpr *Lambda;
  BodyFacts Facts;
  bool LocalClass;
  std::vector<std::unique_ptr<CaptureNode>> Children;
};

/// @param L 要分析的闭包；捕获初始化不作为函数体中的下一层捕获链处理。
/// @return 持有局部事实及直属子闭包的树；不修改 AST 或生成补丁。
std::unique_ptr<CaptureNode> buildTree(LambdaExpr *L) {
  DirectLambdas Visitor;
  Visitor.TraverseStmt(L->getBody());
  auto Node = std::make_unique<CaptureNode>();
  Node->Lambda = L;
  Node->Facts = inspectBody(L->getBody());
  Node->Facts.Unevaluated |= Visitor.Unevaluated;
  Node->LocalClass = Visitor.LocalClass;
  for (auto *Child : Visitor.Children)
    Node->Children.push_back(buildTree(Child));
  return Node;
}

/// @param Node 候选捕获树。
/// @return 任一层有引用或 this 捕获时为 true。类型不再作为候选白名单。
/// 第二轮只剩指针值捕获时返回 false，保持幂等性。
bool needsRewrite(const CaptureNode &Node) {
  for (const auto &C : Node.Lambda->captures())
    if (C.getCaptureKind() == LCK_ByRef || C.getCaptureKind() == LCK_This)
      return true;
  for (const auto &Child : Node.Children)
    if (needsRewrite(*Child))
      return true;
  return false;
}

/// 在读取任何子闭包的字节偏移之前，确认整组的源码范围都可编辑。
/// @param Node 当前捕获子树；R 提供主文件与 token 终点检查。
/// @return 所有层均为非宏主文件范围时 true，否则登记拒绝并返回 false。
bool editableTree(const CaptureNode &Node, RewriteContext &R) {
  auto *L = Node.Lambda;
  if (L->getBeginLoc().isMacroID() || L->getEndLoc().isMacroID() ||
      !R.isMainFile(L->getBeginLoc()) ||
      R.tokenEnd(L->getEndLoc()).isInvalid()) {
    R.Plan.reject("capture: nested lambda has an uneditable/macro range");
    return false;
  }
  for (const auto &Child : Node.Children)
    if (!editableTree(*Child, R))
      return false;
  return true;
}

/// @param L 需要在其前面放置原对象地址的闭包。
/// @param R 当前解析上下文。
/// @param AllowReturn 只复制继承指针时，允许放在同块直属 return 之前。
/// @return 同块的直接表达式、单个自动变量声明或允许的 return；否则 nullptr。
/// 只在确实需要新指针时调用；继承父指针的内层闭包无需移动任何求值。
const Stmt *declarationAnchor(LambdaExpr *L, RewriteContext &R,
                              bool AllowReturn) {
  const Stmt *S = L;
  while (true) {
    auto Parents = R.AST.getParents(*S);
    if (Parents.size() != 1)
      return nullptr;
    if (const auto *Block = Parents[0].get<CompoundStmt>())
      return inspectBody(const_cast<CompoundStmt *>(Block)).HasJump ? nullptr : S;
    if (const auto *Return = Parents[0].get<ReturnStmt>()) {
      auto Blocks = R.AST.getParents(*Return);
      const auto *Block = Blocks.size() == 1 ? Blocks[0].get<CompoundStmt>() : nullptr;
      return AllowReturn && Block &&
                     !inspectBody(const_cast<CompoundStmt *>(Block)).HasJump
                 ? Return : nullptr;
    }
    if (const auto *V = Parents[0].get<VarDecl>()) {
      auto Owners = R.AST.getParents(*V);
      const auto *D = Owners.size() == 1 ? Owners[0].get<DeclStmt>() : nullptr;
      if (!V->hasLocalStorage() || !D || !D->isSingleDecl())
        return nullptr;
      auto Blocks = R.AST.getParents(*D);
      const auto *Block = Blocks.size() == 1 ? Blocks[0].get<CompoundStmt>() : nullptr;
      return Block && !inspectBody(const_cast<CompoundStmt *>(Block)).HasJump ? D : nullptr;
    }
    const auto *Parent = Parents[0].get<Expr>();
    // 不能把取地址提前越过条件求值、赋值、返回或循环等控制/表达式边界。
    if (!Parent || isa<BinaryOperator>(Parent) ||
        isa<ConditionalOperator>(Parent) || isa<LambdaExpr>(Parent))
      return nullptr;
    S = Parent;
  }
}

struct LocalEdit {
  unsigned Offset, Length; // 与整个原始主文件相对，最终统一合入 Root 的替换。
  std::string Text;
};
using PointerMap = std::map<const VarDecl *, std::string>;
using FieldMap = std::map<const FieldDecl *, std::string>;

/// this 的后端适配只支持普通整数字段；收集整棵子树所需字段，
/// 使外层即使只在内层使用成员，也能逐层传递该实例的字段地址。
class MemberUses : public RecursiveASTVisitor<MemberUses> {
  RewriteContext &R;

public:
  bool Valid = true;
  std::vector<MemberExpr *> Uses;
  explicit MemberUses(RewriteContext &R) : R(R) {}
  bool TraverseLambdaExpr(LambdaExpr *L) {
    // 捕获 this 自身的隐式初始化不是一次成员访问；只检查子闭包的计算体。
    return TraverseStmt(L->getBody());
  }
  bool VisitCXXThisExpr(CXXThisExpr *E) {
    auto P = R.AST.getParents(*E);
    auto *M = P.size() == 1 ? P[0].get<MemberExpr>() : nullptr;
    auto *F = M ? dyn_cast<FieldDecl>(M->getMemberDecl()) : nullptr;
    if (!F || M->hasQualifier() || F->isBitField() || !F->getType()->isIntegerType() ||
        F->getType().isVolatileQualified())
      Valid = false;
    else
      Uses.push_back(const_cast<MemberExpr *>(M));
    return true;
  }
};

/// 一组捕获树共享名字和局部补丁；内层不直接写全局 Plan，避免嵌套范围重叠。
class NestedPlanner {
  RewriteContext &R;
  unsigned Begin, End;
  std::vector<LocalEdit> Local;
  std::map<unsigned, std::string> Insertions;
  std::string RootDeclarations;
  const Stmt *RootAnchor = nullptr;
  unsigned Count = 0;

  /// @param Node 当前层；只检查本层引用，子闭包由自己的映射处理。
  /// @param Offset 原始引用字节位置。
  /// @return 是否位于某个直属子闭包内部，供父层排除重复编辑。
  bool insideChild(const CaptureNode &Node, unsigned Offset) const {
    for (const auto &Child : Node.Children) {
      auto *L = Child->Lambda;
      if (Offset >= R.SM.getFileOffset(L->getBeginLoc()) &&
          Offset < R.SM.getFileOffset(R.tokenEnd(L->getEndLoc())))
        return true;
    }
    return false;
  }

public:
  /// @param Root 最外层闭包；输入范围已通过非宏检查。
  /// @param R 共享源码及计划；生命周期覆盖本对象。
  NestedPlanner(LambdaExpr *Root, RewriteContext &R)
      : R(R), Begin(R.SM.getFileOffset(Root->getBeginLoc())),
        End(R.SM.getFileOffset(R.tokenEnd(Root->getEndLoc()))) {}

  /// @param Node 当前捕获层。
  /// @param Inherited 父闭包已按值捕获的原对象指针，键是 canonical VarDecl。
  /// @param IsRoot 是否为最外层，决定前置声明属于全局补丁还是局部组合。
  /// @return 当前层及所有子层均满足前提时为 true，否则记录拒绝并返回 false。
  bool visit(const CaptureNode &Node, const PointerMap &Inherited,
             const FieldMap &InheritedFields,
             bool IsRoot) {
    auto *L = Node.Lambda;
    ++Count;
    if (L->isGenericLambda() || Node.LocalClass || Node.Facts.Unevaluated ||
        L->getBeginLoc().isMacroID() || L->getEndLoc().isMacroID()) {
      R.Plan.reject("capture: nested generic/local-class/macro/unevaluated "
                    "lambda needs review");
      return false;
    }
    if (const auto *A = L->getCallOperator()->getDeclaredReturnType()
                            ->getContainedAutoType())
      if (A->isDecltypeAuto()) {
        R.Plan.reject("capture: decltype(auto) return deduction needs review");
        return false;
      }
    PointerMap Pointers;
    FieldMap Fields;
    std::vector<std::pair<std::string, std::string>> NewPointers;
    bool OnlyInherited = true;
    std::string Captures;
    auto appendCapture = [&](const std::string &Name) {
      if (!Captures.empty())
        Captures += ", ";
      Captures += Name;
    };
    bool HasThis = false;
    for (const auto &C : L->captures()) {
      if (C.capturesThis()) {
        if (C.getCaptureKind() != LCK_This) {
          R.Plan.reject("capture: copying *this needs review");
          return false;
        }
        HasThis = true;
        continue;
      }
      auto *V = C.capturesVariable() ? dyn_cast<VarDecl>(C.getCapturedVar())
                                     : nullptr;
      if (!V || V->isInitCapture() || C.isPackExpansion() ||
          C.getLocation().isMacroID()) {
        R.Plan.reject("capture: init/pack/macro capture needs review");
        return false;
      }
      const auto *Key = V->getCanonicalDecl();
      auto Found = Inherited.find(Key);
      if (C.getCaptureKind() == LCK_ByCopy) {
        // 父层已经把该名字替换为指针时，[name] 不能再找到原捕获。
        // 需要新的初始化捕获适配；不能静默捕获指针或原对象。
        if (Found != Inherited.end()) {
          R.Plan.reject("capture: value capture crosses a rewritten reference boundary");
          return false;
        }
        appendCapture(V->getNameAsString());
        continue;
      }
      auto Q = V->getType().getNonReferenceType();
      if (C.getCaptureKind() != LCK_ByRef ||
          V->getStorageClass() == SC_Register ||
          (Q->isRecordType() && R.hasAddressOperator(Q->getAsCXXRecordDecl()))) {
        R.Plan.reject("capture: unsupported reference or overloaded-address capture");
        return false;
      }
      auto Name = R.freshName(V->getNameAsString() + "Ptr");
      Pointers[Key] = Name;
      appendCapture(Name);
      if (Found == Inherited.end()) {
        OnlyInherited = false;
        NewPointers.emplace_back(Name, "&" + V->getNameAsString());
      } else {
        // Frama-Clang 不能直接解析“再捕获外层捕获字段”的变量。
        // 先从该字段复制为函数体局部指针，再按值捕获局部副本。
        // 只复制地址，既不复制原对象，也不捕获指针局部变量的引用。
        NewPointers.emplace_back(Name, Found->second);
      }
    }
    MemberUses Members(R);
    if (HasThis) {
      Members.TraverseStmt(L->getBody());
      if (!Members.Valid) {
        R.Plan.reject("capture: this requires ordinary non-volatile integer field access");
        return false;
      }
      for (auto *M : Members.Uses) {
        auto *F = cast<FieldDecl>(M->getMemberDecl());
        if (Fields.count(F))
          continue;
        auto Name = R.freshName(F->getNameAsString() + "Ptr");
        Fields[F] = Name;
        appendCapture(Name);
        auto Found = InheritedFields.find(F);
        if (Found == InheritedFields.end()) {
          OnlyInherited = false;
          NewPointers.emplace_back(Name, "&(this->" + F->getNameAsString() + ")");
        } else {
          NewPointers.emplace_back(Name, Found->second);
        }
      }
    }
    if (!NewPointers.empty()) {
      const auto *Anchor = declarationAnchor(L, R, OnlyInherited);
      if (!Anchor || Anchor->getBeginLoc().isMacroID()) {
        R.Plan.reject("capture: nested new pointer requires a direct block "
                      "expression or single automatic declaration");
        return false;
      }
      if (const auto *D = dyn_cast<DeclStmt>(Anchor))
        for (auto *Decl : D->decls())
          if (const auto *V = dyn_cast<VarDecl>(Decl))
            if (Pointers.count(V->getCanonicalDecl())) {
              // 声明符后的初始化式能引用自身，但声明前尚不能使用该名字。
              R.Plan.reject("capture: self capture cannot be hoisted before its declaration");
              return false;
            }
      std::string Declarations;
      for (const auto &[Name, Initializer] : NewPointers)
        Declarations += "auto *" + Name + " = " + Initializer + ";\n" +
                        R.indentation(Anchor->getBeginLoc());
      if (IsRoot) {
        RootAnchor = Anchor;
        RootDeclarations = Declarations;
      } else {
        auto Offset = R.SM.getFileOffset(Anchor->getBeginLoc());
        if (Offset < Begin || Offset >= End) {
          R.Plan.reject("capture: nested pointer would leave the outer body");
          return false;
        }
        // 同一表达式的兄弟闭包可以共享插入位置，按遍历顺序合并成一个前奏。
        Insertions[Offset] += Declarations;
      }
    }
    if (!Pointers.empty() || HasThis) {
      auto Intro = L->getIntroducerRange();
      if (Intro.getBegin().isMacroID() || Intro.getEnd().isMacroID()) {
        R.Plan.reject("capture: nested introducer uses a macro");
        return false;
      }
      unsigned Start = R.SM.getFileOffset(Intro.getBegin());
      Local.push_back({Start,
                       R.SM.getFileOffset(R.tokenEnd(Intro.getEnd())) - Start,
                       "[" + Captures + "]"});
    }
    for (auto *Ref : Node.Facts.References) {
      auto *V = dyn_cast<VarDecl>(Ref->getDecl());
      if (!V || !Pointers.count(V->getCanonicalDecl()))
        continue;
      auto Offset =
          R.SM.getFileOffset(R.SM.getExpansionLoc(Ref->getBeginLoc()));
      if (insideChild(Node, Offset))
        continue;
      if (Ref->getBeginLoc().isMacroID() || Ref->getEndLoc().isMacroID()) {
        R.Plan.reject("capture: nested captured object used through a macro");
        return false;
      }
      unsigned Finish = R.SM.getFileOffset(R.tokenEnd(Ref->getEndLoc()));
      Local.push_back({Offset, Finish - Offset,
                       "(*" + Pointers[V->getCanonicalDecl()] + ")"});
    }
    for (auto *M : Members.Uses) {
      auto Offset = R.SM.getFileOffset(R.SM.getExpansionLoc(M->getBeginLoc()));
      if (insideChild(Node, Offset))
        continue;
      if (M->getBeginLoc().isMacroID() || M->getEndLoc().isMacroID()) {
        R.Plan.reject("capture: member used through a macro");
        return false;
      }
      auto Finish = R.SM.getFileOffset(R.tokenEnd(M->getEndLoc()));
      Local.push_back({Offset, Finish - Offset,
                       "(*" + Fields[cast<FieldDecl>(M->getMemberDecl())] + ")"});
    }
    for (const auto &Child : Node.Children)
      if (!visit(*Child, Pointers, Fields, false))
        return false;
    return true;
  }

  /// 合并并校验捕获树内部的文本编辑，再提交一个根替换和可选根指针前奏。
  /// @param Root 原始最外层闭包，仅用于提交全局源码范围。
  /// @return 无；局部越界或重叠登记拒绝，绝不提交部分成功的根替换。
  void finish(LambdaExpr *Root) {
    for (const auto &[Offset, Text] : Insertions)
      Local.push_back({Offset, 0, Text});
    std::sort(Local.begin(), Local.end(), [](const auto &A, const auto &B) {
      return std::tie(A.Offset, A.Length) < std::tie(B.Offset, B.Length);
    });
    for (size_t N = 0; N < Local.size(); ++N) {
      const auto &E = Local[N];
      if (E.Offset < Begin || E.Offset + E.Length > End ||
          (N && Local[N - 1].Offset + Local[N - 1].Length > E.Offset)) {
        R.Plan.reject("capture: nested local edits overlap or leave the root");
        return;
      }
    }
    auto Text = R.Plan.Original.substr(Begin, End - Begin);
    for (auto E = Local.rbegin(); E != Local.rend(); ++E)
      Text.replace(E->Offset - Begin, E->Length, E->Text);
    // 立即调用闭包的语句可能从 lambda 本身开始；把同位置前奏放进该替换，
    // 不放宽全局计划“插入与替换同起点”的拒绝规则。
    if (RootAnchor && R.SM.getFileOffset(RootAnchor->getBeginLoc()) == Begin)
      Text = RootDeclarations + Text;
    else if (RootAnchor)
      R.Plan.addInsertion(
          R.SM, RootAnchor->getBeginLoc(), "capture-pointers", RootDeclarations,
          20, "Take original-object addresses before the outer lambda",
          "Original object lifetimes remain the caller's responsibility");
    R.Plan.add(R.SM, Root->getBeginLoc(), R.tokenEnd(Root->getEndLoc()),
               "capture", Text,
               "Compose " + std::to_string(Count) +
                   " nested lambdas; materialize local pointer copies at each "
                   "boundary",
               "Nested by-value pointer captures and local aliases; no library "
               "API or ownership added");
  }
};
} // namespace

void planCaptureTreeRewrite(LambdaExpr *L, RewriteContext &R) {
  if (hasEnclosingLambda(L, R.AST))
    return; // 最外层回调已接管；不能为子闭包再提交重叠的全局补丁。
  auto Root = buildTree(L);
  if (!needsRewrite(*Root))
    return;
  if (!editableTree(*Root, R))
    return;
  NestedPlanner Planner(L, R);
  if (Planner.visit(*Root, {}, {}, true))
    Planner.finish(L);
}
} // namespace buddy::framac
