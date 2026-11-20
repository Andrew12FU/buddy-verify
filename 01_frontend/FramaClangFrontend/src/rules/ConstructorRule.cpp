// SPDX-License-Identifier: Apache-2.0
#include "rules/ConstructorRule.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/QualTypeNames.h"
#include <set>
#include "llvm/Support/raw_ostream.h"
using namespace clang;
namespace buddy::framac {
namespace {
bool nameableType(QualType Q) {
  Q = Q.getCanonicalType();
  if (Q->isPointerType() || Q->isReferenceType())
    return nameableType(Q->getPointeeType());
  if (auto *F = Q->getAs<FunctionProtoType>()) {
    if (!nameableType(F->getReturnType())) return false;
    for (auto P : F->param_types()) if (!nameableType(P)) return false;
  }
  if (auto *Tag = Q->getAsTagDecl())
    if (Tag->getAccess() == AS_private || Tag->getAccess() == AS_protected)
      return false;
  if (auto *C = dyn_cast_or_null<ClassTemplateSpecializationDecl>(Q->getAsCXXRecordDecl()))
    for (const auto &A : C->getTemplateArgs().asArray())
      if (A.getKind() == TemplateArgument::Type && !nameableType(A.getAsType()))
        return false;
  return true;
}

// Defaults are emitted in a new declaration scope. Literal/brace defaults have
// no unqualified declaration lookup to rebind. Other defaults stay unsupported.
bool portableDefault(Expr *E) {
  E = E->IgnoreParenImpCasts();
  if (isa<IntegerLiteral, FloatingLiteral, CharacterLiteral, StringLiteral,
          CXXBoolLiteralExpr, CXXNullPtrLiteralExpr>(E))
    return true;
  if (auto *C = dyn_cast<ExprWithCleanups>(E)) return portableDefault(C->getSubExpr());
  if (auto *M = dyn_cast<MaterializeTemporaryExpr>(E)) return portableDefault(M->getSubExpr());
  if (auto *B = dyn_cast<CXXBindTemporaryExpr>(E)) return portableDefault(B->getSubExpr());
  if (auto *L = dyn_cast<InitListExpr>(E)) {
    for (auto *I : L->inits()) if (!portableDefault(I)) return false;
    return true;
  }
  if (auto *C = dyn_cast<CXXConstructExpr>(E)) {
    if (isa<CXXTemporaryObjectExpr>(C)) return false;
    for (auto *A : C->arguments()) if (!portableDefault(A)) return false;
    return true;
  }
  return false;
}
}
void planConstructorRewrite(UsingDecl *D, RewriteContext &R) {
  if (!R.Options.enabled("constructor") || !R.isMainFile(D->getBeginLoc()))
    return;
  bool Inherits = false;
  for (auto *Shadow : D->shadows())
    Inherits |= isa<ConstructorUsingShadowDecl>(Shadow);
  if (!Inherits)
    return;
  auto *Owner = dyn_cast<CXXRecordDecl>(D->getDeclContext());
  // This adapter expands concrete non-default constructor signatures, not every C++
  // constructor. A fieldless, non-template owner avoids changing member
  // initialization/noexcept/constexpr behavior or dependent declarations.
  if (!Owner || Owner->getDescribedClassTemplate() || Owner->isDependentType() ||
      Owner->getNumBases() != 1 || Owner->bases_begin()->isVirtual() ||
      !Owner->field_empty() || D->getAccess() != AS_public) {
    R.Plan.reject("constructor: requires a fieldless concrete single-base owner");
    return;
  }
  auto Base = Owner->bases_begin()->getType();
  auto *Record = Base->getAsCXXRecordDecl();
  if (!Record || !(Record = Record->getDefinition())) {
    R.Plan.reject("constructor: base definition is unavailable");
    return;
  }
  auto *BaseTemplate = dyn_cast<ClassTemplateSpecializationDecl>(Record);
  bool AuditedValueBase = BaseTemplate &&
      BaseTemplate->getSpecializedTemplate()->getQualifiedNameAsString() ==
          "mlir::OpRewritePattern";
  PrintingPolicy Policy(R.Lang);
  Policy.SuppressTagKeyword = true;
  Policy.FullyQualifiedName = true;
  std::string Text;
  bool HasUserConstructor = false;
  for (auto *Ctor : Owner->ctors())
    HasUserConstructor |= !Ctor->isImplicit() && !Ctor->isInheritingConstructor();
  // Adding overloads beside user-defined constructors can change overload
  // ranking. Such owners already have explicit construction logic; leave them.
  if (HasUserConstructor)
    return;
  for (auto *Ctor : Record->ctors()) {
    if (Ctor->getAccess() != AS_public || Ctor->isDeleted() ||
        Ctor->getDescribedFunctionTemplate() || Ctor->isVariadic() ||
        Ctor->isConsteval() || Ctor->isCopyOrMoveConstructor() ||
        Ctor->getNumParams() == 0 || Ctor->getMinRequiredArguments() == 0)
      continue;
    // A generated base initializer passes the entire signature. Reject
    // overloads with the same normalized argument prefix and extra defaults,
    // instead of emitting a base call which is ambiguous even when unused.
    for (auto *Other : Record->ctors()) {
      if (Other == Ctor || Other->isDeleted() || Other->getDescribedFunctionTemplate() ||
          Other->getNumParams() < Ctor->getNumParams() ||
          Other->getMinRequiredArguments() > Ctor->getNumParams())
        continue;
      bool Same = true;
      for (unsigned N = 0; N < Ctor->getNumParams(); ++N)
        Same &= R.AST.hasSameType(
            Ctor->getParamDecl(N)->getType().getNonReferenceType().getLocalUnqualifiedType(),
            Other->getParamDecl(N)->getType().getNonReferenceType().getLocalUnqualifiedType());
      if (Same) {
        R.Plan.reject("constructor: ambiguous base overload prefix requires review");
        return;
      }
    }
    std::string Parameters, Forward;
    bool Supported = true;
    for (unsigned N = 0; N < Ctor->getNumParams(); ++N) {
      auto *Param = Ctor->getParamDecl(N);
      auto Q = Param->getType();
      auto *ValueRecord = Q->getAsCXXRecordDecl();
      // An extra by-value record parameter can change observable identity even
      // when its copy is trivial (e.g. a self pointer created in a prvalue).
      // Only the reviewed value-parameter protocol of the MLIR pattern API is
      // admitted; scalar/pointer/reference signatures remain name-independent.
      bool AuditedValue = AuditedValueBase && ValueRecord &&
          (ValueRecord->getQualifiedNameAsString() == "mlir::PatternBenefit" ||
           ValueRecord->getQualifiedNameAsString() == "llvm::ArrayRef");
      if (Param->hasAttrs() || !nameableType(Q) ||
          (!Q->isReferenceType() &&
           (!Q.isTriviallyCopyableType(R.AST) || (ValueRecord && !AuditedValue)))) {
        Supported = false;
        break;
      }
      auto Name = R.freshName("argument");
      std::string Declaration;
      llvm::raw_string_ostream Stream(Declaration);
      TypeName::getFullyQualifiedType(Q.getCanonicalType(), R.AST).print(Stream, Policy, Name);
      Stream.flush();
      if (Param->hasDefaultArg()) {
        // Declaration-only model headers need not instantiate inherited defaults.
        // A nondependent stored expression still has the original spelling and
        // the same scope checks. Dependent defaults require Sema, so reject them.
        auto *Default = Param->hasUninstantiatedDefaultArg()
                            ? Param->getUninstantiatedDefaultArg()
                            : Param->getDefaultArg();
        if (!Default || Default->isTypeDependent() || Default->isValueDependent() ||
            Default->getBeginLoc().isMacroID() ||
            Default->getEndLoc().isMacroID() || !portableDefault(Default)) {
          Supported = false;
          break;
        }
        auto DefaultText = R.sourceText(Param->getDefaultArgRange());
        llvm::StringRef Spelling(DefaultText);
        Spelling = Spelling.ltrim();
        // Clang's synthesized reference-default range may include '='.
        if (Spelling.consume_front("="))
          Spelling = Spelling.ltrim();
        if (Spelling.empty()) {
          Supported = false;
          break;
        }
        Declaration += " = " + Spelling.str();
      }
      if (!Parameters.empty()) { Parameters += ", "; Forward += ", "; }
      Parameters += Declaration;
      Forward += Q->isRValueReferenceType()
                     ? "static_cast<" + TypeName::getFullyQualifiedName(Q.getCanonicalType(), R.AST, Policy) + ">(" + Name + ")"
                     : Name;
    }
    if (!Supported) {
      R.Plan.reject("constructor: parameter copy/default scope requires review");
      return;
    }
    if (Ctor->hasAttrs() || Ctor->getTrailingRequiresClause()) {
      R.Plan.reject("constructor: constructor attributes/constraints require review");
      return;
    }
    auto I = R.indentation(D->getBeginLoc());
    auto *Proto = Ctor->getType()->getAs<FunctionProtoType>();
    Text += "\n" + I + (Ctor->isExplicit() ? "explicit " : "") +
            (Ctor->isConstexpr() ? "constexpr " : "") + Owner->getNameAsString() +
            "(" + Parameters + ")" + (Proto->isNothrow() ? " noexcept" : "") +
            "\n" + I + "    : " + TypeName::getFullyQualifiedName(Base, R.AST, Policy) +
            "(" + Forward + ") {}";
  }
  if (Text.empty())
    return;
  // A user-provided forwarding constructor suppresses an implicit default.
  // Explicitly defaulting it retains deletion and inferred exception behavior.
  if (!HasUserConstructor && Owner->hasDefaultConstructor() &&
      Record->hasDefaultConstructor())
    Text += "\n" + R.indentation(D->getBeginLoc()) + Owner->getNameAsString() +
            "() = default;";
  auto Token = Lexer::findNextToken(D->getEndLoc(), R.SM, R.Lang);
  if (D->getBeginLoc().isMacroID() || !Token || !Token->is(tok::semi)) {
    R.Plan.reject("constructor: missing editable inherited-constructor terminator");
    return;
  }
  R.Plan.addInsertion(R.SM, R.tokenEnd(Token->getLocation()), "constructor", Text, 10,
      "Forward concrete inherited signatures; preserve defaults at the call site and inherited overloads",
      "No fixed pass, derived class, base specialization or context type");
}
}
