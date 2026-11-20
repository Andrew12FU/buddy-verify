// SPDX-License-Identifier: Apache-2.0
/// @file AST dependency closure for the bounded, shared MiniMLIR library.
/// Source declarations are the only definition source. Keep layout/special
/// member facts and overload families conservatively; remove unused ordinary
/// members.
#include "interface/MiniSlice.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <optional>
#include <set>
#include <vector>

using namespace clang;
namespace buddy::framac {
namespace {
struct Candidate {
  const Decl *D;
  std::string File, Name;
  unsigned Begin, End;
};

/// Clang 18 attaches a written explicit instantiation to its specialization;
/// Clang 23 exposes a separate ExplicitInstantiationDecl wrapper instead.
bool legacyExplicitInstantiation(const Decl *D) {
#if CLANG_VERSION_MAJOR < 23
  if (auto *C = dyn_cast<ClassTemplateSpecializationDecl>(D))
    return C->getSpecializationKind() == TSK_ExplicitInstantiationDefinition ||
           C->getSpecializationKind() == TSK_ExplicitInstantiationDeclaration;
#endif
  return false;
}

/// Canonicalize instantiated declarations to their editable template source.
/// Explicit specializations retain their own source and are not merged away.
const Decl *pattern(const Decl *D) {
  if (!D)
    return nullptr;
  if (auto *F = dyn_cast<FunctionDecl>(D)) {
    if (auto *P = F->getTemplateInstantiationPattern())
      D = P;
    else if (auto *M = dyn_cast<CXXMethodDecl>(F))
      if (auto *P = M->getInstantiatedFromMemberFunction())
        D = P;
    F = cast<FunctionDecl>(D);
    if (F->getTemplateSpecializationKind() != TSK_ExplicitSpecialization)
      if (auto *T = F->getPrimaryTemplate())
        return T;
    if (auto *T = F->getDescribedFunctionTemplate())
      return T;
  }
  if (auto *C = dyn_cast<CXXRecordDecl>(D)) {
    if (auto *S = dyn_cast<ClassTemplateSpecializationDecl>(C)) {
      if (!isa<ClassTemplatePartialSpecializationDecl>(S) &&
          S->getSpecializationKind() != TSK_ExplicitSpecialization) {
        if (auto *Partial = S->getSpecializedTemplateOrPartial()
                                .dyn_cast<ClassTemplatePartialSpecializationDecl *>())
          D = Partial;
        else
          D = S->getSpecializedTemplate()->getTemplatedDecl();
      }
    } else if (auto *P = C->getInstantiatedFromMemberClass()) {
      D = P;
    }
    C = cast<CXXRecordDecl>(D);
    if (auto *T = C->getDescribedClassTemplate())
      return T;
  }
  if (auto *A = dyn_cast<TypeAliasDecl>(D))
    if (auto *T = A->getDescribedAliasTemplate())
      return T;
  return D;
}

class Slice;
/// Inspect one retained declaration's expressions, type spelling and default
/// arguments. Dependency declarations are queued, not recursively traversed
/// here.
class Dependencies : public RecursiveASTVisitor<Dependencies> {
  Slice &S;

public:
  explicit Dependencies(Slice &S) : S(S) {}
  bool VisitDeclRefExpr(DeclRefExpr *E);
  bool VisitMemberExpr(MemberExpr *E);
  bool VisitCallExpr(CallExpr *E);
  bool VisitUnaryOperator(UnaryOperator *E);
  bool VisitCXXConstructExpr(CXXConstructExpr *E);
  bool VisitTypeLoc(TypeLoc T);
  bool VisitUsingShadowDecl(UsingShadowDecl *D);
  bool VisitOverloadExpr(OverloadExpr *E);
  bool VisitCXXDependentScopeMemberExpr(CXXDependentScopeMemberExpr *E);
  bool VisitDependentScopeDeclRefExpr(DependentScopeDeclRefExpr *E);
  bool VisitUnresolvedUsingValueDecl(UnresolvedUsingValueDecl *D);
  bool VisitUnresolvedUsingTypenameDecl(UnresolvedUsingTypenameDecl *D);
  bool VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc T);
  bool TraverseDecl(Decl *D);
  bool TraverseTemplateArgumentLoc(const TemplateArgumentLoc &A);
};

class Slice {
public:
  ASTContext &AST;
  SourceManager &SM;
  std::string Root;
  std::vector<Candidate> Candidates;
  std::set<const Decl *> Kept;
  std::map<const Decl *, std::string> Reasons;
  std::vector<const Decl *> Queue;
  std::set<const Decl *> NeededInstances;
  std::vector<std::pair<const Decl *, const Decl *>> ExplicitInstances;

  Slice(ASTContext &AST, std::string Root)
      : AST(AST), SM(AST.getSourceManager()), Root(std::move(Root)) {}

  bool owned(const Decl *D) const {
    if (!D || D->getLocation().isInvalid())
      return false;
    auto L = SM.getSpellingLoc(D->getLocation());
    llvm::StringRef File = SM.getFilename(L);
    return File.starts_with(Root + "/");
  }

  void memberFamily(DeclarationName Name) {
    for (const auto &C : Candidates)
      if (auto *N = dyn_cast<NamedDecl>(C.D))
        if (N->getDeclContext()->isRecord() && N->getDeclName() == Name)
          keep(N, "dependent member lookup family");
  }

  /// Retain the source pattern and its owning record. All redeclarations and
  /// sibling overloads are kept together to preserve candidate availability.
  void keep(const Decl *Raw, llvm::StringRef Reason) {
    if (owned(Raw))
      NeededInstances.insert(Raw);
    const Decl *D = pattern(Raw);
    if (!owned(D))
      return;
    // A declaration-only member of an instantiated partial specialization may
    // have no instantiation-pattern link. Its spelling still identifies the
    // exact editable member, unlike a name-only match across unrelated types.
    if (isa<CXXMethodDecl>(D)) {
      for (const auto &Candidate : Candidates)
        if (Candidate.D->getKind() == D->getKind() &&
            SM.getSpellingLoc(Candidate.D->getLocation()) ==
                SM.getSpellingLoc(D->getLocation())) {
          D = Candidate.D;
          break;
        }
    }
    if (!Kept.insert(D).second)
      return;
    Reasons[D] = Reason.str();
    Queue.push_back(D);
    // FriendDecl and its namespace-owned function share one source range.
    // Keeping the class's friend wrapper must also keep the actual function;
    // otherwise a second deletion candidate removes the retained declaration.
    if (auto *Friend = dyn_cast<FriendDecl>(D))
      if (auto *Target = Friend->getFriendDecl())
        keep(Target, "function or template declared by retained friend");
    if (auto *U = dyn_cast<UsingDecl>(D))
      for (auto *Shadow : U->shadows())
        keep(Shadow->getTargetDecl(), "using declaration target");
    if (auto *T = dyn_cast<ClassTemplateDecl>(D)) {
      llvm::SmallVector<ClassTemplatePartialSpecializationDecl *> Partials;
      T->getPartialSpecializations(Partials);
      for (auto *P : Partials)
        keep(P, "template partial specialization family");
      for (auto *P : T->specializations())
        if (P->getSpecializationKind() == TSK_ExplicitSpecialization)
          keep(P, "template explicit specialization family");
      for (auto *Redecl : T->redecls())
        if (Redecl != D)
          keep(Redecl, "template redeclaration family");
    }
    if (auto *Owner = dyn_cast<Decl>(D->getDeclContext()))
      if (isa<CXXRecordDecl>(Owner))
        keep(Owner, "owning class of retained member");
    auto *Named = dyn_cast<NamedDecl>(D);
    if (!Named || Named->getDeclName().isEmpty())
      return;
    if (isa<FunctionDecl, FunctionTemplateDecl>(D))
      for (auto *Other : D->getDeclContext()->lookup(Named->getDeclName()))
        if (Other != D)
          keep(Other, "overload family of retained function");
    if (auto *Tag = dyn_cast<TagDecl>(D))
      if (auto *Def = Tag->getDefinition())
        keep(Def, "definition of referenced tag");
  }

  /// Add a source deletion candidate only for a complete, single declaration.
  /// Macro-origin declarations and namespace/linkage/access wrappers stay
  /// intact.
  void candidate(Decl *D) {
    if (!owned(D) || D->isImplicit() ||
        isa<NamespaceDecl, AccessSpecDecl, UsingShadowDecl,
            TemplateTypeParmDecl, NonTypeTemplateParmDecl,
            TemplateTemplateParmDecl, ParmVarDecl>(D))
      return;
    auto *DC = D->getDeclContext();
    if (!DC->isFileContext() && !DC->isRecord())
      return;
    // A retained function body is an indivisible source unit. Do not generate
    // nested deletions for lambda/local-class members inside that body.
    if (auto *Owner = dyn_cast<CXXRecordDecl>(DC))
      if (Owner->isLambda() || Owner->isLocalClass())
        return;
    if (auto *F = dyn_cast<FunctionDecl>(D))
      if (F->getDescribedFunctionTemplate())
        return;
    if (auto *C = dyn_cast<CXXRecordDecl>(D))
      if (C->getDescribedClassTemplate() ||
          (isa<ClassTemplateSpecializationDecl>(C) &&
           cast<ClassTemplateSpecializationDecl>(C)->getSpecializationKind() !=
               TSK_ExplicitSpecialization && !legacyExplicitInstantiation(C)))
        return;
    if (auto *A = dyn_cast<TypeAliasDecl>(D))
      if (A->getDescribedAliasTemplate())
        return;
    auto B = D->getBeginLoc(), E = D->getEndLoc();
    if (B.isMacroID() || E.isMacroID() || SM.getFileID(B) != SM.getFileID(E))
      return;
    E = Lexer::getLocForEndOfToken(E, 0, SM, AST.getLangOpts());
    if (E.isInvalid())
      return;
    unsigned Begin = SM.getFileOffset(B), End = SM.getFileOffset(E);
    auto Buffer = SM.getBufferData(SM.getFileID(B));
    // A trailing semicolon belongs to declarations, not to inline method
    // bodies.
    unsigned Next = End;
    while (Next < Buffer.size() &&
           (Buffer[Next] == ' ' || Buffer[Next] == '\t'))
      ++Next;
    if (Next < Buffer.size() && Buffer[Next] == ';')
      End = Next + 1;
    std::string Name;
    if (auto *N = dyn_cast<NamedDecl>(D))
      Name = N->getQualifiedNameAsString();
    Candidates.push_back({D, SM.getFilename(B).str(), Name, Begin, End});
  }

  /// A class keeps all fields, bases, aliases, special members and virtual
  /// methods. These are structural dependencies, even when no source call names
  /// them.
  void record(CXXRecordDecl *C) {
    if (!C->isCompleteDefinition())
      return;
    for (auto &B : C->bases())
      type(B.getType());
    for (auto *D : C->decls()) {
      auto *F = dyn_cast<CXXMethodDecl>(D);
      auto *T = dyn_cast<FunctionTemplateDecl>(D);
      if (T)
        F = dyn_cast<CXXMethodDecl>(T->getTemplatedDecl());
      if (!F ||
          isa<CXXConstructorDecl, CXXDestructorDecl, CXXConversionDecl>(F) ||
          F->isCopyAssignmentOperator() || F->isMoveAssignmentOperator() ||
          F->isVirtual())
        keep(D,
             "class layout, constraints, special members or virtual surface");
    }
  }

  void type(QualType T) {
    if (T.isNull())
      return;
    if (const auto *A = dyn_cast<TypedefType>(T.getTypePtr()))
      keep(A->getDecl(), "type alias dependency");
    if (const auto *R = T->getAs<RecordType>())
      keep(R->getDecl(), "referenced record type");
    if (const auto *E = T->getAs<EnumType>())
      keep(E->getDecl(), "referenced enum type");
    if (T->isPointerType() || T->isReferenceType())
      type(T->getPointeeType());
  }

  void closure() {
    Dependencies V(*this);
    size_t I = 0;
    while (true) {
      for (; I < Queue.size(); ++I) {
        auto *D = const_cast<Decl *>(Queue[I]);
        if (auto *T = dyn_cast<ClassTemplateDecl>(D)) {
          record(T->getTemplatedDecl());
          for (auto *P : *T->getTemplateParameters())
            V.TraverseDecl(P);
        } else if (auto *C = dyn_cast<CXXRecordDecl>(D)) {
          record(C);
        }
        V.TraverseDecl(D);
      }
      for (auto [Declaration, Specialization] : ExplicitInstances)
        if (NeededInstances.count(Specialization)) {
          // In 18 the editable declaration is also the instantiated record.
          // Traverse its concrete bases and written arguments before retaining
          // its template pattern, without rooting every other instantiation.
          if (legacyExplicitInstantiation(Declaration)) {
            V.TraverseDecl(const_cast<Decl *>(Declaration));
            auto *C = cast<ClassTemplateSpecializationDecl>(Declaration);
            if (auto *Info = C->getTypeAsWritten())
              V.TraverseTypeLoc(Info->getTypeLoc());
          }
          keep(Declaration,
               "explicit instantiation of required structural type");
        }
      if (I == Queue.size())
        break;
    }
  }
};

bool Dependencies::VisitDeclRefExpr(DeclRefExpr *E) {
  S.keep(E->getDecl(), "reference from retained source");
  return true;
}
bool Dependencies::VisitMemberExpr(MemberExpr *E) {
  S.keep(E->getMemberDecl(), "member reference from retained source");
  return true;
}
bool Dependencies::VisitUnaryOperator(UnaryOperator *E) {
  // A dependent dereference in decltype(*declval<Iterator>()) has no resolved
  // callee yet. Keep the operator family used by candidate iterator types;
  // builtin dereference contributes no declaration of its own.
  if (E->getOpcode() == UO_Deref && E->isTypeDependent())
    S.memberFamily(S.AST.DeclarationNames.getCXXOperatorName(OO_Star));
  return true;
}
bool Dependencies::VisitCallExpr(CallExpr *E) {
  S.keep(E->getDirectCallee(), "direct call from retained source");
  return true;
}
bool Dependencies::VisitCXXConstructExpr(CXXConstructExpr *E) {
  S.keep(E->getConstructor(), "constructor dependency");
  return true;
}
bool Dependencies::VisitTypeLoc(TypeLoc T) {
  S.type(T.getType());
  if (auto TL = T.getAs<TypedefTypeLoc>())
    S.keep(TL.getTypePtr()->getDecl(), "type alias dependency");
  return true;
}
bool Dependencies::VisitTemplateSpecializationTypeLoc(
    TemplateSpecializationTypeLoc T) {
  S.keep(T.getTypePtr()->getTemplateName().getAsTemplateDecl(),
         "template dependency");
  return true;
}
bool Dependencies::VisitUsingShadowDecl(UsingShadowDecl *D) {
  S.keep(D->getTargetDecl(), "using declaration target");
  return true;
}
bool Dependencies::VisitOverloadExpr(OverloadExpr *E) {
  for (auto *D : E->decls())
    S.keep(D, "dependent lookup candidate set");
  return true;
}
bool Dependencies::VisitCXXDependentScopeMemberExpr(
    CXXDependentScopeMemberExpr *E) {
  // The primary template has no selected member declaration yet. Keep matching
  // model member families conservatively; never guess a unique instantiation.
  S.memberFamily(E->getMemberNameInfo().getName());
  return true;
}
bool Dependencies::VisitUnresolvedUsingValueDecl(UnresolvedUsingValueDecl *D) {
  S.memberFamily(D->getDeclName());
  return true;
}
bool Dependencies::VisitDependentScopeDeclRefExpr(
    DependentScopeDeclRefExpr *E) {
  S.memberFamily(E->getDeclName());
  return true;
}
bool Dependencies::VisitUnresolvedUsingTypenameDecl(
    UnresolvedUsingTypenameDecl *D) {
  S.memberFamily(D->getDeclName());
  return true;
}
bool Dependencies::TraverseDecl(Decl *D) {
  if (!D)
    return true;
  // Record members are queued individually; traversing a whole class would keep
  // every unused method and defeat member extraction.
  if (auto *C = dyn_cast<CXXRecordDecl>(D)) {
    S.record(C);
    if (C->isCompleteDefinition())
      for (auto &B : C->bases())
        if (auto *Info = B.getTypeSourceInfo())
          TraverseTypeLoc(Info->getTypeLoc());
    if (auto *P = dyn_cast<ClassTemplatePartialSpecializationDecl>(C)) {
      S.keep(P->getSpecializedTemplate(),
             "primary template of retained partial specialization");
      for (auto *Parameter : *P->getTemplateParameters())
        TraverseDecl(Parameter);
      if (auto *Args = P->getTemplateArgsAsWritten())
        for (auto &Arg : Args->arguments())
          TraverseTemplateArgumentLoc(Arg);
    }
    return true;
  }
  if (auto *T = dyn_cast<ClassTemplateDecl>(D)) {
    TraverseDecl(T->getTemplatedDecl());
    for (auto *P : *T->getTemplateParameters())
      TraverseDecl(P);
    return true;
  }
  return RecursiveASTVisitor::TraverseDecl(D);
}
bool Dependencies::TraverseTemplateArgumentLoc(const TemplateArgumentLoc &A) {
  auto Kind = A.getArgument().getKind();
  if (Kind == TemplateArgument::Template ||
      Kind == TemplateArgument::TemplateExpansion)
    S.keep(A.getArgument().getAsTemplateOrTemplatePattern().getAsTemplateDecl(),
           "template template argument dependency");
  return RecursiveASTVisitor::TraverseTemplateArgumentLoc(A);
}

class Inventory : public RecursiveASTVisitor<Inventory> {
  Slice &S;

public:
  explicit Inventory(Slice &S) : S(S) {}
  /// Clang 18's default visitor skips instantiated records, including written
  /// explicit instantiations. Register only those written declarations here;
  /// implicit instantiations have no independent source range to remove.
  bool VisitClassTemplateDecl(ClassTemplateDecl *T) {
#if CLANG_VERSION_MAJOR < 23
    for (auto *Instance : T->specializations())
      if (S.owned(Instance) && legacyExplicitInstantiation(Instance)) {
        S.candidate(Instance);
        S.ExplicitInstances.emplace_back(Instance, Instance);
      }
#endif
    return true;
  }
  bool VisitDecl(Decl *D) {
    S.candidate(D);
#if CLANG_VERSION_MAJOR >= 23
    if (S.owned(D))
      if (auto *E = dyn_cast<ExplicitInstantiationDecl>(D))
        S.ExplicitInstances.emplace_back(E, E->getSpecialization());
#else
    if (S.owned(D) && legacyExplicitInstantiation(D))
      S.ExplicitInstances.emplace_back(D, D);
#endif
    // Namespace-level lookup can affect roots without an explicit reference.
    // A member using declaration matters only after its owner is retained;
    // rooting every inherited constructor would retain every unrelated class.
    if (isa<UsingDecl, UsingShadowDecl, UsingDirectiveDecl, LinkageSpecDecl>(
            D) &&
        D->getDeclContext()->isFileContext())
      S.keep(D, "lookup infrastructure");
    return true;
  }
};
class Roots : public RecursiveASTVisitor<Roots> {
  Slice &S;

public:
  explicit Roots(Slice &S) : S(S) {}
  /// The range syntax hides begin/end, comparison, increment and dereference
  /// calls. They are real interface dependencies even though RAV normally
  /// skips these compiler-generated statements.
  bool VisitCXXForRangeStmt(CXXForRangeStmt *Loop) {
    Dependencies Hidden(S);
    Hidden.TraverseStmt(Loop->getBeginStmt());
    Hidden.TraverseStmt(Loop->getEndStmt());
    // Reconstruct the editable method family from the concrete range's class.
    // Unlike global spelling lookup this remains limited to the selected owner.
    for (auto *EndDecl : {Loop->getBeginStmt(), Loop->getEndStmt()}) {
      auto *EndVar = dyn_cast<VarDecl>(EndDecl->getSingleDecl());
      if (EndVar) Hidden.TraverseStmt(EndVar->getInit());
      auto *Call = EndVar ? dyn_cast_or_null<CXXMemberCallExpr>(
          EndVar->getInit()->IgnoreImplicit()) : nullptr;
      if (Call) {
        auto *Method = Call->getMethodDecl();
        auto *Owner = Method->getParent();
        const Decl *OwnerPattern = pattern(Owner);
        if (auto *Template = dyn_cast<ClassTemplateDecl>(OwnerPattern))
          OwnerPattern = Template->getTemplatedDecl();
        if (auto *Record = dyn_cast<CXXRecordDecl>(OwnerPattern))
          for (auto *Member : Record->lookup(Method->getDeclName()))
            S.keep(Member, "implicit range end lookup on selected owner");
      }
    }
    Hidden.TraverseStmt(Loop->getCond());
    Hidden.TraverseStmt(Loop->getInc());
    Hidden.TraverseStmt(Loop->getLoopVarStmt());
    Hidden.TraverseStmt(Loop->getLoopVariable()->getInit());
    return true;
  }

  /// Tuple decomposition uses compiler lookups not represented by a TypeLoc.
  /// Include that lookup family and the binding expressions (ADL get calls).
  bool VisitDecompositionDecl(DecompositionDecl *D) {
    auto StdName = &S.AST.Idents.get("std");
    for (auto *Std : S.AST.getTranslationUnitDecl()->lookup(StdName))
      if (auto *NS = dyn_cast<NamespaceDecl>(Std))
        for (const char *Name : {"tuple_size", "tuple_element"})
          for (auto *Trait : NS->lookup(&S.AST.Idents.get(Name)))
            S.keep(Trait, "compiler tuple-decomposition lookup");
    Dependencies Hidden(S);
    for (auto *Binding : D->bindings()) {
      S.type(Binding->getType());
      Hidden.TraverseStmt(Binding->getBinding());
      if (auto *Holding = Binding->getHoldingVar()) {
        Hidden.TraverseDecl(Holding);
        Hidden.TraverseStmt(Holding->getInit());
      }
    }
    return true;
  }

  bool TraverseDecl(Decl *D) {
    if (D && D->getLocation().isValid() &&
        !S.SM.isWrittenInMainFile(S.SM.getExpansionLoc(D->getLocation())))
      return true;
    return RecursiveASTVisitor::TraverseDecl(D);
  }
  bool VisitDeclRefExpr(DeclRefExpr *E) {
    S.keep(E->getDecl(), "pass expression root");
    return true;
  }
  bool VisitMemberExpr(MemberExpr *E) {
    S.keep(E->getMemberDecl(), "pass member root");
    return true;
  }
  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    S.keep(E->getConstructor(), "pass constructor root");
    return true;
  }
  bool VisitTypeLoc(TypeLoc T) {
    S.type(T.getType());
    return true;
  }
  bool VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc T) {
    S.keep(T.getTypePtr()->getTemplateName().getAsTemplateDecl(),
           "pass template root");
    return true;
  }
  bool VisitCXXRecordDecl(CXXRecordDecl *D) {
    if (D->isCompleteDefinition())
      for (auto &B : D->bases())
        S.type(B.getType());
    return true;
  }
};

class Consumer : public ASTConsumer {
  std::string Root;
  llvm::json::Object &Result;

public:
  Consumer(std::string Root, llvm::json::Object &Result)
      : Root(std::move(Root)), Result(Result) {}
  void HandleTranslationUnit(ASTContext &AST) override {
    if (AST.getDiagnostics().hasErrorOccurred())
      return;
    Slice S(AST, Root);
    Inventory I(S);
    I.TraverseDecl(AST.getTranslationUnitDecl());
    Roots R(S);
    R.TraverseDecl(AST.getTranslationUnitDecl());
    S.closure();
    llvm::json::Array Declarations;
    for (const auto &C : S.Candidates) {
      const Decl *P = pattern(C.D);
      bool Keep = S.Kept.count(P);
      if (legacyExplicitInstantiation(C.D))
        Keep = S.NeededInstances.count(C.D);
      // Source forward declarations, using directives and access labels are
      // cheap name-lookup infrastructure; they are never rewritten into fake
      // definitions.
      if (auto *T = dyn_cast<TagDecl>(C.D))
        if (!T->isCompleteDefinition() && T->getDeclContext()->isFileContext())
          Keep = true;
      if (C.D->getDeclContext()->isFileContext() &&
          isa<UsingDecl, UsingDirectiveDecl, LinkageSpecDecl>(C.D))
        Keep = true;
      Declarations.push_back(llvm::json::Object{
          {"file", C.File},
          {"name", C.Name},
          {"kind", C.D->getDeclKindName()},
          {"begin", C.Begin},
          {"end", C.End},
          {"keep", Keep},
          {"reason",
           Keep ? (legacyExplicitInstantiation(C.D)
                       ? "explicit instantiation of required structural type"
                       : S.Reasons.count(P) ? S.Reasons[P] : "lookup infrastructure")
                : "outside dependency closure"}});
    }
    Result["schema_version"] = 1;
    Result["parsed"] = true;
    Result["declarations"] = std::move(Declarations);
  }
};
class Action : public ASTFrontendAction {
  std::string Root;
  llvm::json::Object &Result;

public:
  Action(std::string Root, llvm::json::Object &Result)
      : Root(std::move(Root)), Result(Result) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &,
                                                 llvm::StringRef) override {
    return std::make_unique<Consumer>(Root, Result);
  }
};
class Factory : public tooling::FrontendActionFactory {
  std::string Root;
  llvm::json::Object &Result;

public:
  Factory(std::string Root, llvm::json::Object &Result)
      : Root(std::move(Root)), Result(Result) {}
  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<Action>(Root, Result);
  }
};
} // namespace

int sliceMiniLibrary(const tooling::CompilationDatabase &Database,
                     const std::string &Input, const std::string &LibraryRoot,
                     const std::string &Report) {
  if (!llvm::sys::path::is_absolute(LibraryRoot) ||
      !llvm::sys::fs::is_directory(LibraryRoot) ||
      llvm::sys::fs::exists(Report) ||
      llvm::StringRef(Report).starts_with(LibraryRoot + "/"))
    return 2;
  auto Hash = [](llvm::StringRef File) -> std::optional<std::string> {
    auto Buffer = llvm::MemoryBuffer::getFile(File);
    if (!Buffer)
      return std::nullopt;
    llvm::SHA256 State;
    State.update((*Buffer)->getBuffer());
    return llvm::toHex(State.final(), true);
  };
  auto InputHash = Hash(Input);
  if (!InputHash)
    return 2;
  llvm::json::Object Headers;
  std::error_code WalkError;
  for (llvm::sys::fs::recursive_directory_iterator It(LibraryRoot, WalkError),
       End;
       It != End && !WalkError; It.increment(WalkError)) {
    if (!llvm::sys::fs::is_regular_file(It->path()))
      continue;
    auto Value = Hash(It->path());
    if (!Value)
      return 2;
    Headers[It->path()] = *Value;
  }
  if (WalkError)
    return 2;
  llvm::json::Object Result{{"parsed", false},
                            {"library_root", LibraryRoot},
                            {"input", Input},
                            {"input_sha256", *InputHash},
                            {"canonical_headers_sha256", std::move(Headers)}};
  tooling::ClangTool Tool(Database, {Input});
  Factory F(LibraryRoot, Result);
  int Code = Tool.run(&F);
  std::error_code EC;
  llvm::raw_fd_ostream OS(Report, EC);
  if (EC)
    return 2;
  OS << llvm::formatv("{0:2}\n", llvm::json::Value(std::move(Result)));
  return Code ? 1 : 0;
}
} // namespace buddy::framac
