// SPDX-License-Identifier: Apache-2.0
#include "rules/ConstLocalRule.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/QualTypeNames.h"
#include "clang/AST/RecursiveASTVisitor.h"
using namespace clang;
namespace buddy::framac {
namespace {
// The source identifier changes from an object declaration to a reference.
// Reject contexts observing that declaration type or removing constness.
class TypeSensitiveUses : public RecursiveASTVisitor<TypeSensitiveUses> {
public:
  bool Unsafe = false;
  bool VisitDecltypeTypeLoc(DecltypeTypeLoc) { Unsafe = true; return true; }
  bool VisitCXXConstCastExpr(CXXConstCastExpr *) { Unsafe = true; return true; }
  bool VisitFunctionDecl(FunctionDecl *F) {
    if (auto *A = F->getReturnType()->getContainedAutoType())
      Unsafe |= A->isDecltypeAuto();
    return true;
  }
};
}
void planConstLocalRewrite(DeclStmt *D, RewriteContext &R) {
  if (!R.Options.enabled("const-local") || !R.isMainFile(D->getBeginLoc()) ||
      !D->isSingleDecl())
    return;
  auto *V = dyn_cast<VarDecl>(D->getSingleDecl());
  if (!V || !V->hasLocalStorage() || !V->getType().isConstQualified() ||
      !V->hasInit())
    return;
  auto *C = V->getType()->getAsCXXRecordDecl();
  if (!C)
    return;
  auto P = R.AST.getParents(*D);
  TypeSensitiveUses Uses;
  DynTypedNode Node = DynTypedNode::create(*D);
  while (true) {
    auto Parents = R.AST.getParents(Node);
    if (Parents.size() != 1)
      break;
    if (auto *F = Parents[0].get<FunctionDecl>()) {
      Uses.TraverseDecl(const_cast<FunctionDecl *>(F));
      break;
    }
    Node = Parents[0];
  }
  if (P.size() != 1 || !P[0].get<CompoundStmt>() ||
      V->getType().isVolatileQualified() ||
      !V->getType().isTriviallyCopyableType(R.AST) ||
      !C->hasTrivialDestructor() || !C->getIdentifier() ||
      V->isConstexpr() || V->hasAttrs() || Uses.Unsafe ||
      D->getBeginLoc().isMacroID() || V->getInit()->getBeginLoc().isMacroID() ||
      V->getInit()->getEndLoc().isMacroID()) {
    R.Plan.reject("const-local: requires a trivial record directly in a block without declaration-type/constness observers");
    return;
  }
  auto Storage = R.freshName(V->getNameAsString() + "Storage");
  PrintingPolicy Policy(R.Lang);
  Policy.SuppressTagKeyword = true;
  auto Type = TypeName::getFullyQualifiedName(
      V->getType().getLocalUnqualifiedType(), R.AST, Policy);
  auto Text = Type + " " + Storage + " = " +
              R.sourceText(V->getInit()->getSourceRange()) + ";\n" +
              R.indentation(D->getBeginLoc()) + "const " + Type + " &" +
              V->getNameAsString() + " = " + Storage + ";";
  R.Plan.add(R.SM, D->getBeginLoc(), R.statementEnd(D), "const-local", Text,
             "Initialize one trivial record storage and retain a const lvalue under the original name",
             "No pass/type-name whitelist; declaration-type and constness observers reject");
}
} // namespace buddy::framac
