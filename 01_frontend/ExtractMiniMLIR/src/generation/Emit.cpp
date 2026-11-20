// SPDX-License-Identifier: Apache-2.0
#include "generation/Emit.h"
#include "generation/Policy.h"
#include "clang/AST/DeclTemplate.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <functional>
using namespace clang;
namespace buddy::extract {
namespace {
/// Opens namespace scopes only. Nested records need an explicit ordering rule.
bool scope(const DeclContext *dc, llvm::raw_ostream &out, bool close = false) {
  if (dc->isTranslationUnit())
    return true;
  auto *ns = dyn_cast<NamespaceDecl>(dc);
  if (!ns || ns->isAnonymousNamespace())
    return false;
  if (!scope(dc->getParent(), out, close))
    return false;
  if (close)
    out << "}\n";
  else
    out << "namespace " << ns->getName() << " {\n";
  return true;
}
std::string type(QualType t, ASTContext &ast) {
  PrintingPolicy p(ast.getLangOpts());
  p.SuppressTagKeyword = true;
  p.FullyQualifiedName = true;
  return t.getCanonicalType().getAsString(p);
}
bool supported(const TagDecl *d) {
  auto *c = dyn_cast<CXXRecordDecl>(d);
  return (!c || supportsSpecialization(c)) && !d->getDeclContext()->isRecord();
}
/// Emit only concrete declarations. No template body, default argument,
/// noexcept expression or macro is copied as source text. Missing support is
/// diagnosed.
void method(Registry &r, const FunctionDecl *f, llvm::StringRef owner,
            llvm::raw_ostream &o) {
  if (auto *pattern = emptyWalkPattern(f)) {
    auto *t = pattern->getDescribedFunctionTemplate();
    auto *param = t->getTemplateParameters()->getParam(0);
    auto *m = cast<CXXMethodDecl>(pattern);
    o << "template<class " << param->getName() << "> ";
    if (m->isStatic())
      o << "static ";
    if (pattern->isConstexpr())
      o << "constexpr ";
    o << "void walk(" << pattern->getParamDecl(0)->getType().getAsString()
      << " callback)";
    if (m->isConst())
      o << " const";
    if (m->isVolatile())
      o << " volatile";
    if (m->getRefQualifier() == RQ_LValue)
      o << " &";
    if (m->getRefQualifier() == RQ_RValue)
      o << " &&";
    if (pattern->getType()->getAs<FunctionProtoType>()->isNothrow())
      o << " noexcept";
    o << " {} // explicit abstraction: callback is not executed\n";
    return;
  }
  if (f->getPrimaryTemplate() || f->getDescribedFunctionTemplate())
    return;
  auto *m = dyn_cast<CXXMethodDecl>(f);
  if (m && m->isImplicit() &&
      (isa<CXXConstructorDecl>(m) || isa<CXXDestructorDecl>(m) ||
       m->isCopyAssignmentOperator() || m->isMoveAssignmentOperator()))
    return;
  if (f->isConstexpr())
    return;
  if (auto *c = dyn_cast<CXXConstructorDecl>(f))
    if (c->isExplicit())
      o << "explicit ";
  if (auto *c = dyn_cast<CXXConversionDecl>(f))
    if (c->isExplicit())
      o << "explicit ";
  if (m && m->isVirtual())
    o << "virtual ";
  if (m && m->isStatic())
    o << "static ";
  bool ctor = isa<CXXConstructorDecl>(f), dtor = isa<CXXDestructorDecl>(f),
       conv = isa<CXXConversionDecl>(f);
  std::string name = f->getNameAsString();
  if (ctor)
    name = owner.str();
  if (dtor)
    name = "~" + owner.str();
  if (conv)
    name = "operator " +
           type(cast<CXXConversionDecl>(f)->getConversionType(), r.ast);
  std::string declarator = name + "(";
  for (unsigned i = 0; i < f->getNumParams(); ++i) {
    if (i)
      declarator += ", ";
    auto *p = f->getParamDecl(i);
    std::string param;
    llvm::raw_string_ostream s(param);
    PrintingPolicy pp(r.ast.getLangOpts());
    pp.SuppressTagKeyword = true;
    pp.FullyQualifiedName = true;
    p->getType().getCanonicalType().print(s, pp, "arg" + std::to_string(i));
    declarator += s.str();
    if (p->hasDefaultArg())
      r.issue(f, "default_argument",
              "Selected signature has a default expression; reconstruction not "
              "yet supported.");
  }
  if (f->isVariadic())
    declarator += (f->getNumParams() ? ", ..." : "...");
  declarator += ")";
  if (m) {
    if (m->isConst())
      declarator += " const";
    if (m->isVolatile())
      declarator += " volatile";
    if (m->getRefQualifier() == RQ_LValue)
      declarator += " &";
    if (m->getRefQualifier() == RQ_RValue)
      declarator += " &&";
  }
  if (auto *proto = f->getType()->getAs<FunctionProtoType>()) {
    if (proto->isNothrow())
      declarator += " noexcept";
    else if (proto->hasDependentExceptionSpec())
      r.issue(f, "exception_specification",
              "Dependent exception specification requires expression "
              "reconstruction.");
  }
  if (ctor || dtor || conv)
    o << declarator;
  else {
    PrintingPolicy pp(r.ast.getLangOpts());
    pp.SuppressTagKeyword = true;
    pp.FullyQualifiedName = true;
    f->getReturnType().getCanonicalType().print(o, pp, declarator);
  }
  if (m && m->isPureVirtual())
    o << " = 0";
  o << ";\n";
}
} // namespace
bool emit(Registry &r, llvm::StringRef directory) {
  llvm::SmallString<256> inc(directory);
  llvm::sys::path::append(inc, "include");
  if (llvm::sys::fs::create_directories(inc))
    return false;
  llvm::SmallString<256> hp(inc);
  llvm::sys::path::append(hp, "skeleton.h");
  std::error_code ec;
  llvm::raw_fd_ostream out(hp, ec);
  if (ec)
    return false;
  out << "#pragma once\n// Generated solely from real AST usage. External "
         "behavior is unspecified.\n";
  // Primary declarations preserve arity. Definitions are only observed concrete
  // specializations, not a copy of the real generic implementation.
  std::set<std::string> primary;
  for (auto &[k, t] : r.types)
    if (auto *s = dyn_cast<ClassTemplateSpecializationDecl>(t.decl)) {
      if (!supported(s))
        continue;
      auto *p = s->getSpecializedTemplate();
      if (primary.insert(r.id(p)).second && scope(p->getDeclContext(), out)) {
        out << "template <";
        unsigned n = 0;
        for (auto *param : *p->getTemplateParameters()) {
          if (n++)
            out << ", ";
          out << "class " << param->getName();
        }
        out << "> struct " << p->getName() << ";\n";
        scope(p->getDeclContext(), out, true);
      }
    }
  auto localName = [&](const TagDecl *d) {
    if (auto *s = dyn_cast<ClassTemplateSpecializationDecl>(d)) {
      std::string name = d->getNameAsString() + "<";
      unsigned n = 0;
      for (auto &a : s->getTemplateArgs().asArray()) {
        if (n++)
          name += ", ";
        name += type(a.getAsType(), r.ast);
      }
      return name + ">";
    }
    return d->getNameAsString();
  };
  for (auto &[k, d] : r.localForwards)
    if (scope(d->getDeclContext(), out)) {
      out << d->getKindName() << " " << d->getName() << ";\n";
      scope(d->getDeclContext(), out, true);
    }
  // Ordinary names precede specializations whose arguments use those names.
  // A missing unsupported template remains a diagnostic, not a fabricated type.
  for (bool specializationPass : {false, true})
    for (auto &[k, t] : r.types) {
      if (isa<ClassTemplateSpecializationDecl>(t.decl) != specializationPass)
        continue;
      auto *d = t.decl;
      if (!supported(d)) {
        t.generated = "unsupported";
        continue;
      }
      if (!scope(d->getDeclContext(), out)) {
        r.issue(d, "namespace_context",
                "Non-namespace enclosing context not supported.");
        continue;
      }
      if (auto *e = dyn_cast<EnumDecl>(d)) {
        if (e->isScoped())
          out << "enum class ";
        else
          out << "enum ";
        out << e->getName() << " : " << type(e->getIntegerType(), r.ast)
            << ";\n";
      } else {
        if (isa<ClassTemplateSpecializationDecl>(d))
          out << "template <> ";
        out << "struct " << localName(d) << ";\n";
      }
      scope(d->getDeclContext(), out, true);
      t.generated = "forward";
    }
  // Topological value-dependency ordering; method signatures may use forward
  // names.
  std::set<std::string> active, done;
  std::function<void(const std::string &)> definition =
      [&](const std::string &k) {
        if (done.count(k))
          return;
        auto &t = r.types.at(k);
        auto *d = t.decl;
        if (!supported(d) || t.state == State::Forward)
          return;
        if (!active.insert(k).second) {
          r.issue(d, "complete_type_cycle",
                  "By-value field cycle cannot be ordered.");
          return;
        }
        for (auto &[key, m] : t.members)
          if (auto *f = dyn_cast<FieldDecl>(m))
            if (auto *tag = f->getType()->getAsTagDecl())
              if (r.types.count(r.id(tag->getCanonicalDecl())))
                definition(r.id(tag->getCanonicalDecl()));
        active.erase(k);
        done.insert(k);
        t.generated = "defined";
        if (!scope(d->getDeclContext(), out))
          return;
        if (auto *e = dyn_cast<EnumDecl>(d)) {
          out << (e->isScoped() ? "enum class " : "enum ") << e->getName()
              << " : " << type(e->getIntegerType(), r.ast) << " {\n";
          for (auto *v : e->enumerators())
            out << v->getName() << " = " << v->getInitVal() << ",\n";
          out << "};\n";
        } else {
          if (isa<ClassTemplateSpecializationDecl>(d))
            out << "template <> ";
          out << "struct " << localName(d) << " {\n";
          AccessSpecifier access = AS_none;
          for (auto &[key, m] : t.members) {
            if (m->getAccess() != access) {
              access = m->getAccess();
              out << (access == AS_private     ? "private"
                      : access == AS_protected ? "protected"
                                               : "public")
                  << ":\n";
            }
            if (auto *f = dyn_cast<FunctionDecl>(m))
              method(r, f, d->getName(), out);
            else if (auto *f = dyn_cast<FieldDecl>(m)) {
              PrintingPolicy p(r.ast.getLangOpts());
              p.SuppressTagKeyword = true;
              p.FullyQualifiedName = true;
              f->getType().getCanonicalType().print(out, p, f->getName());
              out << ";\n";
            }
          }
          out << "};\n";
        }
        scope(d->getDeclContext(), out, true);
      };
  for (auto &[k, t] : r.types)
    definition(k);
  for (auto &[k, a] : r.aliases) {
    if (!scope(a->getDeclContext(), out)) {
      r.issue(a, "alias_context", "Member alias needs enclosing-type support.");
      continue;
    }
    out << "using " << a->getName() << " = "
        << type(a->getUnderlyingType(), r.ast) << ";\n";
    scope(a->getDeclContext(), out, true);
  }
  for (auto &[k, f] : r.functions) {
    if (scope(f->getDeclContext(), out)) {
      method(r, f, "", out);
      scope(f->getDeclContext(), out, true);
    } else
      r.issue(f, "function_context", "Unsupported function context.");
  }
  std::set<std::string> emittedImports;
  for (auto &[k, u] : r.imports) {
    auto *target = u->getTargetDecl();
    std::string text = "using ::" + target->getQualifiedNameAsString() + ";";
    std::string key =
        std::to_string(reinterpret_cast<uintptr_t>(u->getDeclContext())) + text;
    if (emittedImports.insert(key).second) {
      if (scope(u->getDeclContext(), out)) {
        out << text << "\n";
        scope(u->getDeclContext(), out, true);
      } else
        r.issue(u, "using_context",
                "Member using declaration needs an enclosing type.");
    }
  }
  out.close();
  llvm::SmallString<256> jp(directory);
  llvm::sys::path::append(jp, "records.json");
  llvm::raw_fd_ostream json(jp, ec);
  if (ec)
    return false;
  json << llvm::formatv("{0:2}\n", llvm::json::Value(r.report()));
  return !out.has_error() && !json.has_error();
}
} // namespace buddy::extract
