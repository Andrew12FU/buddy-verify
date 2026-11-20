// SPDX-License-Identifier: Apache-2.0
#include "analysis/Collect.h"
#include "clang/AST/RecursiveASTVisitor.h"
using namespace clang;
namespace buddy::extract {
namespace {
class Uses : public RecursiveASTVisitor<Uses> {
  Registry &r;
  bool main(SourceLocation l) {
    auto &sm = r.ast.getSourceManager();
    return l.isValid() && sm.isWrittenInMainFile(sm.getExpansionLoc(l));
  }

public:
  explicit Uses(Registry &v) : r(v) {}
  bool TraverseDecl(Decl *d) {
    if (d && d->getLocation().isValid() && !main(d->getLocation()))
      return true;
    return RecursiveASTVisitor::TraverseDecl(d);
  }
  bool VisitTypeLoc(TypeLoc t) {
    if (main(t.getBeginLoc()))
      if (auto alias = t.getAs<TypedefTypeLoc>()) {
        auto *d = alias.getTypedefNameDecl();
        if (r.external(d))
          r.aliases.emplace(r.id(d), d);
      }
    if (main(t.getBeginLoc()))
      r.need(t.getType(), false, "explicit type name");
    return true;
  }
  bool VisitExpr(Expr *e) {
    if (auto *c = dyn_cast<CastExpr>(e))
      if (receiverOnlyCast(r.ast, c))
        return true;
    if (main(e->getExprLoc()) && !e->getType().isNull())
      r.need(e->getType(), !e->getType()->isPointerType(), "expression type");
    return true;
  }
  bool VisitCXXMethodDecl(CXXMethodDecl *m) {
    if (!main(m->getLocation()))
      return true;
    for (auto *f : m->overridden_methods()) {
      const CXXRecordDecl *receiver = nullptr;
      for (auto &b : m->getParent()->bases())
        if (auto *c = b.getType()->getAsCXXRecordDecl()) {
          if (c->getCanonicalDecl() == f->getParent()->getCanonicalDecl() ||
              c->isDerivedFrom(f->getParent())) {
            receiver = c;
            break;
          }
        }
      r.interface(f, receiver, m->getLocation(), "override obligation");
    }
    return true;
  }
  bool VisitDeclRefExpr(DeclRefExpr *e) {
    if (main(e->getExprLoc()))
      if (auto *s = dyn_cast<UsingShadowDecl>(e->getFoundDecl()))
        r.imports.emplace(r.id(s), s);
    if (main(e->getExprLoc()))
      if (auto *f = dyn_cast<FunctionDecl>(e->getDecl()))
        r.interface(f, nullptr, e->getExprLoc(), "function reference");
    return true;
  }
  bool VisitCallExpr(CallExpr *e) {
    if (!main(e->getExprLoc()))
      return true;
    const CXXRecordDecl *receiver = nullptr;
    if (auto *m = dyn_cast<CXXMemberCallExpr>(e)) {
      if (auto *obj = m->getImplicitObjectArgument()) {
        auto t = obj->IgnoreParenImpCasts()->getType();
        if (t->isPointerType())
          t = t->getPointeeType();
        receiver = t->getAsCXXRecordDecl();
      }
    }
    r.interface(e->getDirectCallee(), receiver, e->getExprLoc(), "call");
    return true;
  }
  bool VisitCXXConstructExpr(CXXConstructExpr *e) {
    if (main(e->getExprLoc()))
      r.interface(e->getConstructor(), nullptr, e->getExprLoc(),
                  "construction/copy/move");
    return true;
  }
  bool VisitImplicitCastExpr(ImplicitCastExpr *e) {
    if (main(e->getExprLoc()))
      recordCast(r, e);
    return true;
  }
  bool VisitCXXBindTemporaryExpr(CXXBindTemporaryExpr *e) {
    if (main(e->getExprLoc()))
      r.interface(e->getTemporary()->getDestructor(), nullptr, e->getExprLoc(),
                  "temporary destruction");
    return true;
  }
  bool VisitVarDecl(VarDecl *v) {
    if (main(v->getLocation()))
      r.need(v->getType(), true, "variable declaration");
    if (main(v->getLocation()))
      if (auto *c = v->getType()->getAsCXXRecordDecl())
        if (auto *d = c->getDestructor())
          r.interface(d, nullptr, v->getLocation(), "scope destruction");
    return true;
  }
  bool VisitMemberExpr(MemberExpr *e) {
    if (!main(e->getExprLoc()))
      return true;
    if (auto *f = dyn_cast<FieldDecl>(e->getMemberDecl())) {
      if (r.external(f)) {
        auto t = e->getBase()->IgnoreParenImpCasts()->getType();
        if (t->isPointerType())
          t = t->getPointeeType();
        if (auto *c = t->getAsCXXRecordDecl()) {
          r.need(t, true, "field access");
          r.need(f->getType(), true, "field type");
          r.types[r.id(c->getCanonicalDecl())].members.emplace(r.id(f), f);
          auto o = r.source(f);
          o["generated_owner"] = r.id(c);
          o["abstraction"] = "accessed field only; not real layout";
          r.uses.push_back(std::move(o));
        }
      }
    }
    return true;
  }
};
} // namespace
void collect(Registry &r) {
  Uses(r).TraverseDecl(r.ast.getTranslationUnitDecl());
}
} // namespace buddy::extract
