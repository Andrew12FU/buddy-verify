// SPDX-License-Identifier: Apache-2.0
#include "rules/RangeForRule.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <algorithm>
#include <map>
using namespace clang;
namespace buddy::framac {
namespace {
/// Gather all nested loops; the outer loop owns their combined replacement.
class Loops : public RecursiveASTVisitor<Loops> {
public:
  std::vector<CXXForRangeStmt *> Nodes;
  bool VisitCXXForRangeStmt(CXXForRangeStmt *S) { Nodes.push_back(S); return true; }
};
/// The enumerate index is a per-iteration proxy member in LLVM. Mutating or
/// escaping it cannot be translated into modification of the loop counter.
class IndexUse : public RecursiveASTVisitor<IndexUse> {
  const BindingDecl *Index;
  bool namesIndex(const Expr *E) const {
    auto *Ref = dyn_cast<DeclRefExpr>(E->IgnoreParenImpCasts());
    return Ref && Ref->getDecl() == Index;
  }
public:
  bool Unsafe = false;
  explicit IndexUse(const BindingDecl *Index) : Index(Index) {}
  bool VisitUnaryOperator(UnaryOperator *E) {
    if ((E->isIncrementDecrementOp() || E->getOpcode() == UO_AddrOf) &&
        namesIndex(E->getSubExpr())) Unsafe = true;
    return true;
  }
  bool VisitBinaryOperator(BinaryOperator *E) {
    if (E->isAssignmentOp() && namesIndex(E->getLHS())) Unsafe = true;
    return true;
  }
  bool VisitVarDecl(VarDecl *V) {
    if (V->getType()->isReferenceType() && V->hasInit() && namesIndex(V->getInit()))
      Unsafe = true;
    return true;
  }
  bool VisitCallExpr(CallExpr *Call) {
    auto *F = Call->getDirectCallee();
    for (unsigned N = 0; N < Call->getNumArgs(); ++N) {
      if (!namesIndex(Call->getArg(N))) continue;
      unsigned Parameter = N;
      if (isa<CXXOperatorCallExpr>(Call))
        if (auto *Method = dyn_cast_or_null<CXXMethodDecl>(F))
          if (!Method->isStatic() && Parameter) --Parameter;
      if (!F || Parameter >= F->getNumParams()) { Unsafe = true; continue; }
      auto T = F->getParamDecl(Parameter)->getType();
      if (T->isReferenceType() && !T.getNonReferenceType().isConstQualified())
        Unsafe = true;
    }
    return true;
  }
};

/// @return Whether an enclosing range loop will own this node's replacement.
bool nested(CXXForRangeStmt *S, ASTContext &AST) {
  DynTypedNode N = DynTypedNode::create(*S);
  while (true) {
    auto P = AST.getParents(N);
    if (P.size() != 1) return false;
    if (P[0].get<CXXForRangeStmt>()) return true;
    if (P[0].get<FunctionDecl>()) return false;
    N = P[0];
  }
}
}
void planRangeForRewrite(CXXForRangeStmt *S, RewriteContext &R) {
  if (!R.Options.enabled("range-for") || !R.isMainFile(S->getBeginLoc()) ||
      nested(S, R.AST)) return;
  Loops L; L.TraverseStmt(S);
  struct Patch { unsigned B, E; std::string Text; };
  std::vector<Patch> Patches;
  std::map<unsigned, std::string> Closings;
  for (auto *Loop : L.Nodes) {
    auto *V = Loop->getLoopVariable();
    auto *Init = Loop->getRangeInit();
    if (Loop->getInit() || Loop->getBeginLoc().isMacroID() ||
        Init->getBeginLoc().isMacroID() || Init->getEndLoc().isMacroID() ||
        inspectBody(Loop->getBody()).HasJump) {
      R.Plan.reject("range-for: macro/init-statement/goto requires review"); return;
    }
    auto I = R.indentation(Loop->getBeginLoc());
    auto Range = R.freshName("rangeValues");
    auto It = R.freshName("rangeIt"), End = R.freshName("rangeEnd");
    std::string Header, Binding;
    if (auto *D = dyn_cast<DecompositionDecl>(V)) {
      // This deliberately handles only llvm::enumerate over an indexed range;
      // zip and arbitrary tuple decomposition need their own audited rules.
      auto *Call = dyn_cast<CallExpr>(Init->IgnoreImplicit());
      auto *Callee = Call ? Call->getDirectCallee() : nullptr;
      if (!Callee || Callee->getQualifiedNameAsString() != "llvm::enumerate" ||
          Call->getNumArgs() != 1 || D->bindings().size() != 2) {
        R.Plan.reject("range-for: only two-element llvm::enumerate is audited"); return;
      }
      auto *SourceRange = Call->getArg(0)->getType()->getAsCXXRecordDecl();
      IndexUse Uses(D->bindings()[0]);
      Uses.TraverseStmt(Loop->getBody());
      if (!SourceRange || SourceRange->getQualifiedNameAsString() != "llvm::ArrayRef" ||
          V->getType()->isReferenceType() || V->getType().isConstQualified() ||
          Uses.Unsafe || inspectBody(Loop->getBody()).NestedLambda) {
        R.Plan.reject("range-for: enumerate requires an ArrayRef view and nonescaping read-only index"); return;
      }
      auto Index = D->bindings()[0]->getNameAsString();
      auto Element = D->bindings()[1]->getNameAsString();
      auto Size = R.freshName("rangeSize");
      Header = "{\n" + I + "  auto " + Range + " = " +
          R.sourceText(Call->getArg(0)->getSourceRange()) + ";\n" + I +
          "  size_t " + Size + " = " + Range + ".size();\n" + I +
          "  for (size_t " + Index + " = 0; " + Index + " < " + Size +
          "; ++" + Index + ") {\n";
      Binding = "auto &&" + Element + " = " + Range + "[" + Index + "];";
    } else {
      auto *BeginVar = cast<VarDecl>(Loop->getBeginStmt()->getSingleDecl());
      if (!isa<CXXMemberCallExpr>(BeginVar->getInit()->IgnoreImplicit())) {
        R.Plan.reject("range-for: array/ADL begin needs a separate audited rule"); return;
      }
      // Keep the original range expression, including early_inc adapters.
      // auto&& evaluates it once and extends a temporary range's lifetime.
      Header = "{\n" + I + "  auto &&" + Range + " = " +
          R.sourceText(Init->getSourceRange()) + ";\n" + I +
          "  auto " + It + " = " + Range + ".begin();\n" + I +
          "  auto " + End + " = " + Range + ".end();\n" + I +
          "  for (; " + It + " != " + End + "; ++" + It + ") {\n";
      // Preserve explicit reference/copy declaration, replacing its implicit
      // compiler-generated initializer with dereference of the same iterator.
      auto T = V->getType().getAsString();
      if (V->getTypeSourceInfo())
        T = R.sourceText(V->getTypeSourceInfo()->getTypeLoc().getSourceRange());
      Binding = T + " " + V->getNameAsString() + " = *" + It + ";";
    }
    Header += I + "    " + Binding + "\n" + I + "    ";
    Patches.push_back({R.SM.getFileOffset(Loop->getBeginLoc()),
                      R.SM.getFileOffset(Loop->getBody()->getBeginLoc()), Header});
    auto E = R.SM.getFileOffset(R.statementEnd(Loop->getBody()));
    // Parents are visited first. Prepending each nested close preserves scope.
    Closings[E] = "\n" + I + "  }\n" + I + "}" + Closings[E];
  }
  for (const auto &[E, Text] : Closings) Patches.push_back({E, E, Text});
  std::sort(Patches.begin(), Patches.end(), [](const auto &A, const auto &B) {
    return A.B > B.B;
  });
  auto B = R.SM.getFileOffset(S->getBeginLoc());
  auto E = R.SM.getFileOffset(R.statementEnd(S));
  auto Text = R.Plan.Original.substr(B, E-B);
  for (const auto &P : Patches) Text.replace(P.B-B, P.E-P.B, P.Text);
  R.Plan.add(R.SM, S->getBeginLoc(), R.statementEnd(S), "range-for", Text,
             "Desugar " + std::to_string(L.Nodes.size()) +
             " range loops; preserve one-time evaluation, lifetime and early increment");
}
}
