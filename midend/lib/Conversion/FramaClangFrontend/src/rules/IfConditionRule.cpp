// SPDX-License-Identifier: Apache-2.0
#include "rules/IfConditionRule.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
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
/// admitted by the imported profiles; observable destructor ordering rejects.
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
} // namespace

void planIfConditionRewrite(IfStmt *S, RewriteContext &R) {
  if (!R.Options.enabled("if-condition") ||
      (R.Options.Profile != "silu-fusion" && R.Options.Profile != "assume-tight" &&
       R.Options.Profile != "eliminate-copy" && R.Options.Profile != "gpu-memcpy") ||
      !R.isMainFile(S->getBeginLoc()) ||
      !S->getConditionVariable())
    return;
  if (R.Options.Profile != "silu-fusion") {
    // An outer condition rewrite owns its original body bytes. Defer nested
    // condition declarations to a fresh AST round rather than overlap edits.
    DynTypedNode N = DynTypedNode::create(*S);
    while (true) {
      auto P = R.AST.getParents(N);
      if (P.size() != 1 || P[0].get<FunctionDecl>()) break;
      if (auto *Outer = P[0].get<IfStmt>())
        if (Outer->getConditionVariable()) return;
      N = P[0];
    }
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
  // render 返回一个完整外层块。原 then/else 体保持嵌套，因此局部遮蔽、
  // break/continue/return 的归属不变；条件对象在整条原 if 结束时离开作用域。
  std::function<std::string(IfStmt *, std::string)> Render;
  auto OriginalIndent = R.indentation(S->getBeginLoc());
  Render = [&](IfStmt *Node, std::string I) -> std::string {
    auto *V = Node->getConditionVariable();
    TemporaryCleanup Cleanup;
    if (V && V->getInit()) Cleanup.TraverseStmt(V->getInit());
    if (!V || Node->getInit() || Node->isConstexpr() ||
        Node->getBeginLoc().isMacroID() || V->getBeginLoc().isMacroID() ||
        V->getInit()->getBeginLoc().isMacroID() ||
        V->getInit()->getEndLoc().isMacroID() ||
        (R.Options.Profile == "silu-fusion" &&
         !isa<CompoundStmt>(Node->getThen())) ||
        Node->getThen()->getBeginLoc().isMacroID() ||
        Node->getThen()->getEndLoc().isMacroID() ||
        (Node->getElse() && (Node->getElse()->getBeginLoc().isMacroID() ||
                             Node->getElse()->getEndLoc().isMacroID())) ||
        V->getType()->isReferenceType() || V->getType().isVolatileQualified() ||
        V->getType().isDestructedType() != QualType::DK_none ||
        (R.Options.Profile == "silu-fusion" &&
         isa<ExprWithCleanups>(V->getInit())) || Cleanup.Nontrivial || inspectBody(Node).HasJump) {
      R.Plan.reject("if-condition: unsupported body/initializer/lifetime/jump: " +
                    V->getNameAsString() + " cleanup=" +
                    std::to_string(isa<ExprWithCleanups>(V->getInit())));
      return {};
    }
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
