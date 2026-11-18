// SPDX-License-Identifier: Apache-2.0
#include "rules/ConstLocalRule.h"
#include "clang/AST/ParentMapContext.h"
using namespace clang;
namespace buddy::framac {
void planConstLocalRewrite(DeclStmt *D, RewriteContext &R) {
  if (!R.Options.enabled("const-local") ||
      (R.Options.Profile != "depthwise-conv" &&
       R.Options.Profile != "builtin-transpose") ||
      !R.isMainFile(D->getBeginLoc()) || !D->isSingleDecl())
    return;
  auto *V = dyn_cast<VarDecl>(D->getSingleDecl());
  if (!V || !V->hasLocalStorage() || !V->getType().isConstQualified() ||
      !V->hasInit())
    return;
  auto *C = V->getType()->getAsCXXRecordDecl();
  if (!C || (C->getQualifiedNameAsString() != "mlir::Value" &&
             C->getQualifiedNameAsString() != "mlir::AffineExpr"))
    return;
  auto P = R.AST.getParents(*D);
  if (P.size() != 1 || !P[0].get<CompoundStmt>() ||
      V->getType().isVolatileQualified() || !C->hasTrivialDestructor() ||
      D->getBeginLoc().isMacroID() || V->getInit()->getEndLoc().isMacroID()) {
    R.Plan.reject("const-local: requires a trivial handle directly in a block");
    return;
  }
  // No new handle object is copied: the storage is the original initialization,
  // and the original identifier remains a const lvalue referring to that
  // object. This is bounded to this reviewed pass;
  // decltype/const_cast/address-escape transformations are not claimed for
  // arbitrary source programs.
  auto Storage = R.freshName(V->getNameAsString() + "Storage");
  auto Type = C->getQualifiedNameAsString();
  auto Text = Type + " " + Storage + " = " +
              R.sourceText(V->getInit()->getSourceRange()) + ";\n" +
              R.indentation(D->getBeginLoc()) + "const " + Type + " &" +
              V->getNameAsString() + " = " + Storage + ";";
  R.Plan.add(R.SM, D->getBeginLoc(), R.statementEnd(D), "const-local", Text,
             "Initialize mutable handle storage and retain a const lvalue "
             "under the original name",
             "Same handle initialization; const reference aliases its storage "
             "until block exit");
}
} // namespace buddy::framac
