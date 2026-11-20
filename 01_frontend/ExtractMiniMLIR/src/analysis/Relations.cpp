// SPDX-License-Identifier: Apache-2.0
#include "analysis/Collect.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ParentMapContext.h"
using namespace clang;
namespace buddy::extract {
bool receiverOnlyCast(ASTContext &a, const CastExpr *e) {
  if (e->path_empty())
    return false;
  auto parents = a.getParents(*e);
  if (parents.size() != 1)
    return false;
  if (auto *m = parents[0].get<MemberExpr>())
    return m->getBase() == e && isa<CXXMethodDecl>(m->getMemberDecl());
  return false;
}
void recordCast(Registry &r, const CastExpr *e) {
  auto &sm = r.ast.getSourceManager();
  auto l = sm.getExpansionLoc(e->getBeginLoc());
  llvm::json::Array path;
  for (auto *b : e->path())
    path.push_back(b->getType().getAsString());
  bool receiver = receiverOnlyCast(r.ast, e);
  std::string selected, sourceType = e->getSubExpr()->getType().getAsString();
  if (auto *call =
          dyn_cast<CXXMemberCallExpr>(e->getSubExpr()->IgnoreParenImpCasts())) {
    if (auto *f = call->getDirectCallee())
      selected = r.id(f);
    if (auto *obj = call->getImplicitObjectArgument())
      sourceType = obj->IgnoreParenImpCasts()->getType().getAsString();
  } else if (auto *ctor = dyn_cast<CXXConstructExpr>(
                 e->getSubExpr()->IgnoreParenImpCasts()))
    selected = r.id(ctor->getConstructor());
  r.relations.push_back(llvm::json::Object{
      {"kind", e->getCastKindName()},
      {"source_type", sourceType},
      {"selected_decl", selected},
      {"receiver_only", receiver},
      {"target_type", e->getType().getAsString()},
      {"line", sm.getSpellingLineNumber(l)},
      {"real_base_path", std::move(path)},
      {"generation_support",
       receiver          ? "base member flattened to actual receiver"
       : e->path_empty() ? "selected operation/signature tracked separately"
                         : "standalone inheritance conversion unsupported"}});
  if (!receiver)
    r.need(e->getType(), false, "implicit operation target");
  if (!e->path_empty() && !receiver) {
    // Record actual inheritance conversions instead of inventing arbitrary
    // conversions. Receiver-only casts can disappear after flattening, but
    // argument conversions still need a future context-sensitive rule. Do not
    // assert support here.
    if (auto *d = e->getSubExpr()->getType()->getAsCXXRecordDecl())
      r.issue(d, "conversion",
              "Actual derived/base cast observed; receiver flattening "
              "supported, standalone conversion not synthesized yet.");
  }
}
} // namespace buddy::extract
