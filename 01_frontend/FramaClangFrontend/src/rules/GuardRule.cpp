// SPDX-License-Identifier: Apache-2.0
#include "rules/GuardRule.h"
#include "clang/AST/ParentMapContext.h"

using namespace clang;
namespace buddy::framac {
namespace {
/// Only the built-in logical-or has the bool result and short-circuit
/// semantics used here. Keep cleanup wrappers in the emitted source: the
/// complete original condition remains one full-expression.
const BinaryOperator *runtimeOr(const IfStmt *S) {
  if (S->isConstexpr() || !S->getCond())
    return nullptr;
  const Expr *Condition = S->getCond();
  while (true) {
    Condition = Condition->IgnoreParenImpCasts();
    if (const auto *Cleanup = dyn_cast<ExprWithCleanups>(Condition))
      Condition = Cleanup->getSubExpr();
    else
      break;
  }
  auto *Or = dyn_cast<BinaryOperator>(Condition);
  return Or && Or->getOpcode() == BO_LOr ? Or : nullptr;
}
} // namespace

void planGuardRewrite(IfStmt *S, RewriteContext &R) {
  if (!R.Options.enabled("guard") || !R.isMainFile(S->getBeginLoc()) ||
      !runtimeOr(S))
    return;

  // An outer replacement owns the original branch bytes. Rewrite nested and
  // else-if conditions on fresh AST rounds instead of submitting overlapping
  // edits. Include lambda/local-class bodies: their source bytes can also be
  // contained in an outer if's replacement, despite separate execution scopes.
  std::vector<DynTypedNode> Pending{DynTypedNode::create(*S)};
  std::set<const void *> Seen;
  while (!Pending.empty()) {
    auto Node = Pending.back();
    Pending.pop_back();
    for (const auto &Parent : R.AST.getParents(Node)) {
      if (const auto *Outer = Parent.get<IfStmt>())
        if (runtimeOr(Outer))
          return;
      auto *Key = Parent.getMemoizationData();
      if (Key && Seen.insert(Key).second)
        Pending.push_back(Parent);
    }
  }

  auto *Condition = S->getCond();
  auto *Init = S->getInit();
  if (S->getBeginLoc().isMacroID() || S->getEndLoc().isMacroID() ||
      Condition->getBeginLoc().isMacroID() ||
      Condition->getEndLoc().isMacroID() ||
      (Init && (Init->getBeginLoc().isMacroID() ||
                Init->getEndLoc().isMacroID())) ||
      inspectBody(S).HasJump) {
    R.Plan.reject("guard: macro or goto/label scope requires review");
    return;
  }

  auto End = R.statementEnd(S);
  auto ConditionEnd = R.tokenEnd(Condition->getEndLoc());
  if (End.isInvalid() || ConditionEnd.isInvalid()) {
    R.Plan.reject("guard: uneditable if or condition range");
    return;
  }
  auto Begin = R.SM.getFileOffset(S->getBeginLoc());
  auto ConditionBegin = R.SM.getFileOffset(Condition->getBeginLoc());
  auto Text = R.Plan.Original.substr(Begin, R.SM.getFileOffset(End) - Begin);
  auto Name = R.freshName("guardFailed");
  auto ConditionText = R.sourceText(Condition->getSourceRange());
  // Replace only the condition. All then/else statements, braces, comments and
  // literal contents retain their original bytes, irrespective of their shape.
  Text.replace(ConditionBegin - Begin,
               R.SM.getFileOffset(ConditionEnd) - ConditionBegin, Name);

  std::string InitText;
  if (Init) {
    auto InitEnd = R.statementEnd(Init);
    if (InitEnd.isInvalid()) {
      R.Plan.reject("guard: uneditable if initializer");
      return;
    }
    auto InitBegin = R.SM.getFileOffset(Init->getBeginLoc());
    auto Length = R.SM.getFileOffset(InitEnd) - InitBegin;
    InitText = R.Plan.Original.substr(InitBegin, Length);
    // This range precedes the replaced condition, so original offsets remain
    // valid. The initializer lives through both branches in the new block.
    Text.erase(InitBegin - Begin, Length);
  }

  auto I = R.indentation(S->getBeginLoc());
  auto Replacement = "{\n" + I + "  ";
  if (!InitText.empty())
    Replacement += InitText + "\n" + I + "  ";
  Replacement += "bool " + Name + " = (" + ConditionText + ");\n" + I +
                 "  " + Text + "\n" + I + "}";
  R.Plan.add(R.SM, S->getBeginLoc(), End, "guard", Replacement,
             "Bind a built-in || condition once; preserve the complete if and "
             "both branches independently of profile, method and return value",
             "One fresh block-local bool; if-init lifetime and short-circuit "
             "evaluation remain scoped to the original if");
}
} // namespace buddy::framac
