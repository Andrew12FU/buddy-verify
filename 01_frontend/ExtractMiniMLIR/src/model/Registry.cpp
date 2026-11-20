// SPDX-License-Identifier: Apache-2.0
#include "model/Registry.h"
#include "generation/Policy.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Version.h"
#include "clang/Index/USRGeneration.h"
using namespace clang;
namespace buddy::extract {
std::string Registry::id(const Decl *d) const {
  llvm::SmallString<256> b;
  if (!index::generateUSRForDecl(d, b))
    return b.str().str();
  auto &sm = ast.getSourceManager();
  auto l = sm.getSpellingLoc(d->getLocation());
  std::string signature;
  if (auto *v = dyn_cast<ValueDecl>(d))
    signature = v->getType().getAsString();
  auto *n = dyn_cast<NamedDecl>(d);
  return std::string(d->getDeclKindName()) + ":" +
         (n ? n->getQualifiedNameAsString() : "") + ":" + signature + ":" +
         sm.getFilename(l).str() + ":" + std::to_string(sm.getFileOffset(l));
}
llvm::json::Object Registry::source(const Decl *d) const {
  auto &sm = ast.getSourceManager();
  auto l = sm.getSpellingLoc(d->getLocation());
  auto *n = dyn_cast<NamedDecl>(d);
  return llvm::json::Object{{"id", id(d)},
                            {"name", n ? n->getQualifiedNameAsString() : ""},
                            {"kind", d->getDeclKindName()},
                            {"file", sm.getFilename(l).str()},
                            {"line", sm.getSpellingLineNumber(l)},
                            {"macro", d->getLocation().isMacroID()}};
}
bool Registry::external(const Decl *d) const {
  auto &sm = ast.getSourceManager();
  return d && !sm.isWrittenInMainFile(sm.getExpansionLoc(d->getLocation()));
}
void Registry::issue(const Decl *d, llvm::StringRef category,
                     llvm::StringRef why) {
  if (!issueKeys.insert(id(d) + ":" + category.str() + ":" + why.str()).second)
    return;
  auto o = source(d);
  o["category"] = category;
  o["reason"] = why;
  unsupported.push_back(std::move(o));
}
void Registry::need(QualType t, bool complete, llvm::StringRef why) {
  if (t.isNull())
    return;
  if (auto *u = t->getAs<UsingType>())
    imports.emplace(id(u->getFoundDecl()), u->getFoundDecl());
  if (auto *s = t->getAs<TemplateSpecializationType>())
    if (auto *u = s->getTemplateName().getAsUsingShadowDecl())
      imports.emplace(id(u), u);
  if (auto *p = t->getAs<PointerType>()) {
    need(p->getPointeeType(), false, why);
    return;
  }
  if (auto *p = t->getAs<ReferenceType>()) {
    need(p->getPointeeType(), false, why);
    return;
  }
  if (auto *p = ast.getAsArrayType(t)) {
    need(p->getElementType(), true, why);
    return;
  }
  if (auto *p = t->getAs<FunctionProtoType>()) {
    need(p->getReturnType(), true, why);
    for (auto a : p->param_types())
      need(a, true, why);
    return;
  }
  const TagDecl *d = t->getAsTagDecl();
  if (!d)
    return;
  if (!external(d)) {
    if (why.starts_with("template argument"))
      if (auto *c = dyn_cast<CXXRecordDecl>(d))
        localForwards.emplace(id(c), c);
    return;
  }
  d = cast<TagDecl>(d->getCanonicalDecl());
  auto [i, fresh] = types.try_emplace(id(d));
  auto &rec = i->second;
  rec.decl = d;
  rec.reasons.insert(why.str());
  if (complete)
    rec.state = State::Complete;
  if (!fresh)
    return;
  if (d->getDeclContext()->isRecord() && isa<EnumDecl>(d))
    issue(d, "nested_type",
          "Nested enum needs enclosing-type definition placement.");
  if (auto *c = dyn_cast<CXXRecordDecl>(d)) {
    llvm::json::Array bases;
    if (auto *def = c->getDefinition())
      for (auto &b : def->bases())
        bases.push_back(b.getType().getAsString());
    relations.push_back(llvm::json::Object{{"kind", "real_bases_not_restored"},
                                           {"type", id(d)},
                                           {"bases", std::move(bases)}});
    if (auto *s = dyn_cast<ClassTemplateSpecializationDecl>(c)) {
      if (!supportsSpecialization(c))
        issue(d, "template_definition",
              "Class template requires defaults, constraints, packs, or "
              "non-type/template parameters beyond current "
              "concrete-specialization support.");
      for (auto &a : s->getTemplateArgs().asArray())
        if (a.getKind() == TemplateArgument::Type)
          need(a.getAsType(), true, "template argument of " + id(d));
    }
    if (c->getDeclContext()->isRecord())
      issue(d, "nested_type",
            "Nested type requires an enclosing skeleton and declaration "
            "ordering.");
  }
}
void Registry::interface(const FunctionDecl *f, const CXXRecordDecl *receiver,
                         SourceLocation at, llvm::StringRef operation) {
  if (!f || !external(f))
    return;
  auto o = source(f);
  auto &sm = ast.getSourceManager();
  at = sm.getExpansionLoc(at);
  o["use_line"] = sm.getSpellingLineNumber(at);
  o["use_column"] = sm.getSpellingColumnNumber(at);
  o["operation"] = operation;
  o["signature"] = f->getType().getAsString();
  o["return_type"] = f->getReturnType().getAsString();
  llvm::json::Array params;
  for (auto *p : f->parameters())
    params.push_back(
        llvm::json::Object{{"type", p->getType().getAsString()},
                           {"default_argument", p->hasDefaultArg()}});
  o["parameters"] = std::move(params);
  o["real_context"] =
      f->getDeclContext()->isTranslationUnit()
          ? "::"
          : Decl::castFromDeclContext(f->getDeclContext())->getDeclKindName();
  o["output"] = "include/skeleton.h";
  o["output_status"] =
      emptyWalkPattern(f) ? "empty callback definition"
      : (f->getPrimaryTemplate() || f->getDescribedFunctionTemplate())
          ? "unsupported function template"
      : f->isConstexpr() ? "unsupported constexpr definition"
      : f->isImplicit()  ? "compiler-synthesized special member"
                         : "declaration";
  const auto *m = dyn_cast<CXXMethodDecl>(f);
  const CXXRecordDecl *owner =
      receiver ? receiver : (m ? m->getParent() : nullptr);
  if (owner && !external(owner))
    owner = m ? m->getParent() : nullptr;
  if (m) {
    o["const"] = m->isConst();
    o["volatile"] = m->isVolatile();
    o["static"] = m->isStatic();
    o["ref_qualifier"] = m->getRefQualifier() == RQ_LValue   ? "&"
                         : m->getRefQualifier() == RQ_RValue ? "&&"
                                                             : "";
    o["access"] = m->getAccess() == AS_private     ? "private"
                  : m->getAccess() == AS_protected ? "protected"
                                                   : "public";
    o["real_owner"] = id(m->getParent());
    o["generated_owner"] = owner ? id(owner) : "";
    o["abstraction"] = (owner && owner->getCanonicalDecl() !=
                                     m->getParent()->getCanonicalDecl())
                           ? "flatten inherited member to receiver"
                           : "declaration-only member";
    need(ast.getRecordType(owner), true, "member access");
    types[id(owner->getCanonicalDecl())].members.emplace(id(f), f);
  } else {
    functions.emplace(id(f), f);
    o["abstraction"] = "declaration-only function";
  }
  uses.push_back(std::move(o));
  need(f->getReturnType(), true, "return signature of " + id(f));
  for (auto *p : f->parameters())
    need(p->getType(), true, "parameter signature of " + id(f));
  if (emptyWalkPattern(f)) {
    relations.push_back(llvm::json::Object{
        {"kind", "callback_not_executed"},
        {"selected", id(f)},
        {"line", sm.getSpellingLineNumber(at)},
        {"rule", "void MLIR walk with one unconstrained callback parameter"}});
  } else if (f->getPrimaryTemplate() || f->getDescribedFunctionTemplate())
    issue(
        f, "template_definition",
        "Selected function template requires template parameters, constraints "
        "and any deduction definition; no fabricated overload emitted.");
  if (f->isConstexpr())
    issue(f, "constexpr_definition",
          "Required constexpr definition not reconstructed yet.");
}
llvm::json::Object Registry::report() const {
  llvm::json::Array ts, as, locals, imps;
  for (auto &[key, t] : types) {
    auto o = source(t.decl);
    o["generated_name"] =
        ast.getTypeDeclType(t.decl).getCanonicalType().getAsString();
    o["state"] = t.state == State::Complete ? "complete skeleton" : "forward";
    o["generation_state"] = t.generated;
    llvm::json::Array reasons, members;
    for (auto &v : t.reasons)
      reasons.push_back(v);
    for (auto &[k, m] : t.members)
      members.push_back(source(m));
    o["reasons"] = std::move(reasons);
    o["members"] = std::move(members);
    ts.push_back(std::move(o));
  }
  for (auto &[k, a] : aliases)
    as.push_back(source(a));
  for (auto &[k, u] : imports) {
    auto o = source(u);
    o["target"] = id(u->getTargetDecl());
    imps.push_back(std::move(o));
  }
  for (auto &[k, d] : localForwards)
    locals.push_back(source(d));
  return llvm::json::Object{
      {"clang_version", CLANG_VERSION_STRING},
      {"imports", std::move(imps)},
      {"types", std::move(ts)},
      {"aliases", std::move(as)},
      {"local_type_forwards", std::move(locals)},
      {"interfaces", llvm::json::Array(uses)},
      {"relations", llvm::json::Array(relations)},
      {"unsupported", llvm::json::Array(unsupported)},
      {"generation_source", "real Clang AST only; no existing MiniMLIR input"},
      {"callback_boundary",
       "No callback execution model; supported walk templates are empty only "
       "when explicitly emitted and recorded."}};
}
} // namespace buddy::extract
