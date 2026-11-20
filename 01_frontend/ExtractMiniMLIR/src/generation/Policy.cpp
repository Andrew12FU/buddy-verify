// SPDX-License-Identifier: Apache-2.0
#include "generation/Policy.h"
#include "clang/AST/DeclTemplate.h"
using namespace clang;
namespace buddy::extract {
bool supportsSpecialization(const CXXRecordDecl *d) {
  auto *s = dyn_cast<ClassTemplateSpecializationDecl>(d);
  if (!s)
    return true;
  auto *params = s->getSpecializedTemplate()->getTemplateParameters();
  if (params->getRequiresClause())
    return false;
  for (auto *p : *params) {
    auto *t = dyn_cast<TemplateTypeParmDecl>(p);
    if (!t || t->isParameterPack() || t->hasDefaultArgument() ||
        t->hasTypeConstraint())
      return false;
  }
  for (auto &a : s->getTemplateArgs().asArray())
    if (a.getKind() != TemplateArgument::Type ||
        a.getAsType()->isDependentType())
      return false;
  return true;
}
const FunctionDecl *emptyWalkPattern(const FunctionDecl *f) {
  if (!isa<CXXMethodDecl>(f) || f->getNameAsString() != "walk" ||
      f->getQualifiedNameAsString().rfind("mlir::", 0) != 0)
    return nullptr;
  auto *t = f->getPrimaryTemplate();
  if (!t)
    t = f->getDescribedFunctionTemplate();
  if (!t)
    return nullptr;
  auto *params = t->getTemplateParameters();
  if (params->size() != 1 || params->getRequiresClause())
    return nullptr;
  auto *p = dyn_cast<TemplateTypeParmDecl>(params->getParam(0));
  if (!p || p->getName().empty() || p->hasDefaultArgument() ||
      p->isParameterPack() || p->hasTypeConstraint())
    return nullptr;
  auto *pattern = t->getTemplatedDecl();
  if (pattern->getNumParams() != 1 || !pattern->getReturnType()->isVoidType() ||
      pattern->getTrailingRequiresClause() ||
      pattern->getParamDecl(0)->hasDefaultArg() ||
      pattern->getType()
          ->getAs<FunctionProtoType>()
          ->hasDependentExceptionSpec())
    return nullptr;
  auto type = pattern->getParamDecl(0)->getType().getNonReferenceType();
  auto *tp = type->getAs<TemplateTypeParmType>();
  return tp && tp->getDepth() == p->getDepth() &&
                 tp->getIndex() == p->getIndex()
             ? pattern
             : nullptr;
}
} // namespace buddy::extract
