// SPDX-License-Identifier: Apache-2.0
#include "rules/IfConditionRule.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <algorithm>
#include <functional>

using namespace clang;
namespace buddy::framac {
namespace {
/// 多行字面量内部的空白属于内容，重排语句缩进时必须保留其原始字节。
class LiteralInventory : public RecursiveASTVisitor<LiteralInventory> {
public:
  std::vector<SourceRange> Ranges;
  bool VisitStringLiteral(StringLiteral *S) {
    Ranges.push_back(S->getSourceRange());
    return true;
  }
  bool VisitCharacterLiteral(CharacterLiteral *S) {
    Ranges.push_back(S->getSourceRange());
    return true;
  }
};

/// A hoisted condition declaration ends its initialization full-expression
/// before the explicit bool conversion. Only trivial temporary cleanup is
/// admitted; observable destructor ordering rejects in every supported profile.
class TemporaryCleanup : public RecursiveASTVisitor<TemporaryCleanup> {
public:
  bool Nontrivial = false;
  bool VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *E) {
    Nontrivial |= E->getType().isDestructedType() != QualType::DK_none;
    return true;
  }
};

/// @param Body 要保留的原分支，包含末尾分号。
/// @param Before/After 原条件链和新 if 所在行的缩进；只替换后续行的公共前缀。
/// @param R 源码上下文；宏展开字面量不作为主文件的原文字节范围处理。
/// @return 只调整语句前导空白、保留多行字面量内容的原分支副本。
std::string branchText(Stmt *Body, llvm::StringRef Before,
                       llvm::StringRef After, RewriteContext &R) {
  auto Begin = R.SM.getFileOffset(Body->getBeginLoc());
  auto End = R.SM.getFileOffset(R.statementEnd(Body));
  auto Text = R.Plan.Original.substr(Begin, End - Begin);
  LiteralInventory Literals;
  Literals.TraverseStmt(Body);
  // 从后向前改缩进，使字面量和源码位置仍对应原始缓冲区的偏移。
  for (size_t N = Text.size(); N > 0; --N) {
    if (Text[N - 1] != '\n')
      continue;
    bool Protected = false;
    for (auto Range : Literals.Ranges) {
      if (Range.getBegin().isMacroID() || Range.getEnd().isMacroID())
        continue;
      Protected |= R.SM.getFileOffset(Range.getBegin()) < Begin + N &&
                   Begin + N < R.SM.getFileOffset(R.tokenEnd(Range.getEnd()));
    }
    if (!Protected && llvm::StringRef(Text).substr(N).starts_with(Before))
      Text.replace(N, Before.size(), After.str());
  }
  return Text;
}

/// 检查整条条件声明链，再决定本轮是补块还是展开。不能先输出不安全的规范化。
bool validCondition(IfStmt *Node, RewriteContext &R) {
  auto *V = Node->getConditionVariable();
  TemporaryCleanup Cleanup;
  if (V && V->hasInit())
    Cleanup.TraverseStmt(V->getInit());
  if (!V || !V->hasInit() || Node->getInit() || Node->isConstexpr() ||
      Node->getBeginLoc().isMacroID() || V->getBeginLoc().isMacroID() ||
      V->getInit()->getBeginLoc().isMacroID() ||
      V->getInit()->getEndLoc().isMacroID() ||
      Node->getThen()->getBeginLoc().isMacroID() ||
      Node->getThen()->getEndLoc().isMacroID() ||
      (Node->getElse() && (Node->getElse()->getBeginLoc().isMacroID() ||
                          Node->getElse()->getEndLoc().isMacroID())) ||
      V->getType()->isReferenceType() || V->getType().isVolatileQualified() ||
      V->getType().isDestructedType() != QualType::DK_none ||
      Cleanup.Nontrivial || inspectBody(Node).HasJump) {
    R.Plan.reject("if-condition: unsupported body/initializer/lifetime/jump");
    return false;
  }
  return true;
}

/// 一条 else-if 链共享一份规范化补丁。只包住完整分支，保留 else-if 连接关系。
/// 下一轮重新解析后，新增的花括号才会成为 CompoundStmt。
bool normalizeBranches(const std::vector<IfStmt *> &Chain, IfStmt *Root,
                       RewriteContext &R) {
  struct BranchEdit {
    unsigned Begin, End;
    std::string Text;
  };
  std::vector<BranchEdit> Edits;
  for (auto *Node : Chain) {
    auto I = R.indentation(Node->getBeginLoc());
    for (auto *Body : {Node->getThen(), Node->getElse()}) {
      if (!Body || isa<CompoundStmt>(Body) ||
          (Body == Node->getElse() && isa<IfStmt>(Body)))
        continue;
      Edits.push_back({R.SM.getFileOffset(Body->getBeginLoc()),
                       R.SM.getFileOffset(R.statementEnd(Body)),
                       "{\n" + I + "  " + branchText(Body, I, I + "  ", R) +
                           "\n" + I + "}"});
    }
  }
  if (Edits.empty())
    return false;
  // 子 else-if 自身不提交补丁，按源码倒序组合整条链的互不重叠分支。
  std::sort(Edits.begin(), Edits.end(),
            [](const auto &A, const auto &B) { return A.Begin > B.Begin; });
  auto Begin = R.SM.getFileOffset(Root->getBeginLoc());
  auto End = R.statementEnd(Root);
  auto Text = R.Plan.Original.substr(Begin, R.SM.getFileOffset(End) - Begin);
  for (const auto &E : Edits)
    Text.replace(E.Begin - Begin, E.End - E.Begin, E.Text);
  R.Plan.add(R.SM, Root->getBeginLoc(), End, "if-condition-normalize", Text,
             "Brace complete branches before desugaring on a fresh AST; keep else-if chains",
             "No new evaluation or library interface");
  return true;
}
} // namespace

void planIfConditionRewrite(IfStmt *S, RewriteContext &R) {
  if (!R.Options.enabled("if-condition") ||
      !R.isMainFile(S->getBeginLoc()) ||
      !S->getConditionVariable())
    return;
  // 所有 profile 使用相同的祖先检查。外层替换拥有内层的原文字节，
  // 一般嵌套留到下一轮；直接 else-if 由根节点的 Render 递归接管。
  DynTypedNode N = DynTypedNode::create(*S);
  while (true) {
    auto P = R.AST.getParents(N);
    if (P.size() != 1 || P[0].get<FunctionDecl>()) break;
    if (auto *Outer = P[0].get<IfStmt>())
      if (Outer->getConditionVariable()) return;
    N = P[0];
  }
  auto Parents = R.AST.getParents(*S);
  // else-if 条件对象必须只在前一条件为假时构造，不能统一提前声明。
  // 根 if 负责整条链；子 else-if 不生成与根重叠的补丁。
  if (Parents.size() == 1 && Parents[0].get<IfStmt>() &&
      Parents[0].get<IfStmt>()->getElse() == S &&
      Parents[0].get<IfStmt>()->getConditionVariable())
    return;
  if (Parents.size() != 1 || !Parents[0].get<CompoundStmt>()) {
    R.Plan.reject("if-condition: root must be a direct block child");
    return;
  }
  std::vector<IfStmt *> Chain;
  for (auto *Node = S; Node;) {
    if (!validCondition(Node, R))
      return;
    Chain.push_back(Node);
    auto *Next = dyn_cast_or_null<IfStmt>(Node->getElse());
    Node = Next && Next->getConditionVariable() ? Next : nullptr;
  }
  if (normalizeBranches(Chain, S, R))
    return; // 本轮只补块，禁止用旧 AST 继续展开同一根 if。
  // render 返回一个完整外层块。原 then/else 体保持嵌套，因此局部遮蔽、
  // break/continue/return 的归属不变；条件对象在整条原 if 结束时离开作用域。
  std::function<std::string(IfStmt *, std::string)> Render;
  auto OriginalIndent = R.indentation(S->getBeginLoc());
  Render = [&](IfStmt *Node, std::string I) -> std::string {
    auto *V = Node->getConditionVariable();
    auto Text = "{\n" + I + "  " + R.sourceText(V->getSourceRange()) + ";\n" +
                I + "  if (static_cast<bool>(" + V->getNameAsString() + ")) " +
                branchText(Node->getThen(), OriginalIndent, I + "  ", R);
    if (auto *Else = Node->getElse()) {
      Text += " else ";
      if (auto *Next = dyn_cast<IfStmt>(Else);
          Next && Next->getConditionVariable())
        Text += Render(Next, I + "  ");
      else
        Text += branchText(Else, OriginalIndent, I + "  ", R);
    }
    return Text + "\n" + I + "}";
  };
  auto Text = Render(S, R.indentation(S->getBeginLoc()));
  R.Plan.add(R.SM, S->getBeginLoc(), R.statementEnd(S), "if-condition", Text,
             "Preserve conditional construction, else priority and body scopes",
             "Explicit contextual bool; no new library interface");
}
} // namespace buddy::framac
