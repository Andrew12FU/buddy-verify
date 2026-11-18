// SPDX-License-Identifier: Apache-2.0
#include "rules/ConstructorRule.h"
#include "clang/Lex/Lexer.h"
using namespace clang;
namespace buddy::framac {
void planConstructorRewrite(UsingDecl *D, RewriteContext &R) {
  if (!R.Options.enabled("constructor") || R.Options.Profile != "eliminate-copy" ||
      !R.isMainFile(D->getBeginLoc())) return;
  auto *Owner = dyn_cast<CXXRecordDecl>(D->getDeclContext());
  if (!Owner || Owner->getNameAsString() != "EliminateMemRefCopyPattern") return;
  for (auto *Ctor : Owner->ctors())
    if (!Ctor->isImplicit() && !Ctor->isInheritingConstructor())
      return; // The second round already has the explicit audited constructor.
  if (Owner->getNumBases() != 1 || D->getAccess() != AS_public ||
      D->getBeginLoc().isMacroID()) {
    R.Plan.reject("constructor: expected public inherited context constructor"); return;
  }
  auto Base = Owner->bases_begin()->getType();
  auto *Record = dyn_cast_or_null<ClassTemplateSpecializationDecl>(Base->getAsCXXRecordDecl());
  if (!Record || Record->getSpecializedTemplate()->getQualifiedNameAsString() !=
                     "mlir::OpRewritePattern" || Record->getTemplateArgs().size() < 1 ||
      Record->getTemplateArgs()[0].getKind() != TemplateArgument::Type ||
      !Record->getTemplateArgs()[0].getAsType()->getAsCXXRecordDecl() ||
      Record->getTemplateArgs()[0].getAsType()->getAsCXXRecordDecl()->getQualifiedNameAsString() !=
                     "mlir::memref::CopyOp") {
    R.Plan.reject("constructor: base specialization changed; audit required"); return;
  }
  auto Token = Lexer::findNextToken(D->getEndLoc(), R.SM, R.Lang);
  if (!Token || !Token->is(tok::semi)) {
    R.Plan.reject("constructor: missing editable using terminator"); return;
  }
  auto I = R.indentation(D->getBeginLoc());
  auto Context = R.freshName("context");
  auto Text = "\n" + I + "explicit EliminateMemRefCopyPattern(mlir::MLIRContext *" +
      Context + ")\n" + I + "    : mlir::OpRewritePattern<mlir::memref::CopyOp>(" + Context + ") {}";
  R.Plan.addInsertion(R.SM, R.tokenEnd(Token->getLocation()), "constructor", Text, 10,
      "Explicitly forward the used context overload; retain all inherited overloads",
      "No new external interface or pattern behavior");
}
}
