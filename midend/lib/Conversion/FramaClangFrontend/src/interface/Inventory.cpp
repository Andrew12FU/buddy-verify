// SPDX-License-Identifier: Apache-2.0
#include "interface/Analysis.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#if __has_include("clang/UnifiedSymbolResolution/USRGeneration.h")
#include "clang/UnifiedSymbolResolution/USRGeneration.h"
#else
#include "clang/Index/USRGeneration.h"
#endif
#include "clang/Lex/Lexer.h"
#include <set>
using namespace clang;
namespace buddy::header_pilot {
std::string identity(const Decl *D) {
  llvm::SmallString<256> Buffer;
  if (!index::generateUSRForDecl(D, Buffer))
    return Buffer.str().str();
  const auto *Named = dyn_cast<NamedDecl>(D);
  return Named ? Named->getQualifiedNameAsString() : D->getDeclKindName();
}
llvm::json::Object describeDecl(ASTContext &AST, const Decl *D) {
  auto &SM = AST.getSourceManager();
  auto L = SM.getSpellingLoc(D->getLocation());
  bool Macro = D->getBeginLoc().isMacroID() || D->getEndLoc().isMacroID();
  std::string Text;
  if (!Macro)
    Text = Lexer::getSourceText(
               CharSourceRange::getTokenRange(D->getSourceRange()), SM,
               AST.getLangOpts())
               .str();
  llvm::json::Object O{{"id", identity(D)},
                       {"kind", D->getDeclKindName()},
                       {"file", SM.getFilename(L).str()},
                       {"line", SM.getSpellingLineNumber(L)},
                       {"column", SM.getSpellingColumnNumber(L)},
                       {"macro_range", Macro},
                       {"text", Text}};
  if (const auto *N = dyn_cast<NamedDecl>(D))
    O["name"] = N->getQualifiedNameAsString();
  return O;
}
namespace {
/// 只走主文件声明和方法体；不递归真实库的全部实现。
class Inventory : public RecursiveASTVisitor<Inventory> {
  ASTContext &AST;
  ProbeResult &Result;
  std::set<std::string> SeenTypes;
  bool main(SourceLocation L) {
    auto &SM = AST.getSourceManager();
    return L.isValid() && SM.isWrittenInMainFile(SM.getExpansionLoc(L));
  }
  /// 记录实际选中的函数，不从函数名猜测重载；隐式构造和默认实参单列。
  void call(const Expr *E, const FunctionDecl *F, llvm::StringRef Kind,
            unsigned Count, unsigned Defaults) {
    if (!main(E->getBeginLoc()))
      return;
    auto &SM = AST.getSourceManager();
    auto L = SM.getExpansionLoc(E->getBeginLoc());
    llvm::json::Object O{
        {"kind", Kind},
        {"line", SM.getSpellingLineNumber(L)},
        {"column", SM.getSpellingColumnNumber(L)},
        {"argument_count", Count},
        {"default_arguments", Defaults},
        {"result_type", E->getType().getCanonicalType().getAsString()},
        {"macro", E->getBeginLoc().isMacroID()}};
    if (F) {
      O["selected"] = describeDecl(AST, F);
      O["signature"] = F->getType().getCanonicalType().getAsString();
      if (const auto *Pattern = F->getTemplateInstantiationPattern())
        O["template_pattern"] = describeDecl(AST, Pattern);
    } else {
      O["indirect"] = true;
    }
    Result.Calls.push_back(std::move(O));
  }

public:
  Inventory(ASTContext &AST, ProbeResult &Result) : AST(AST), Result(Result) {}
  bool TraverseDecl(Decl *D) {
    if (D && D->getLocation().isValid() && !main(D->getLocation()))
      return true;
    return RecursiveASTVisitor::TraverseDecl(D);
  }
  bool VisitCallExpr(CallExpr *E) {
    unsigned Defaults = 0;
    for (const auto *A : E->arguments())
      Defaults += isa<CXXDefaultArgExpr>(A);
    call(E, E->getDirectCallee(), "call", E->getNumArgs(), Defaults);
    return true;
  }
  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    unsigned Defaults = 0;
    for (const auto *A : E->arguments())
      Defaults += isa<CXXDefaultArgExpr>(A);
    call(E, E->getConstructor(), "constructor", E->getNumArgs(), Defaults);
    return true;
  }
  bool VisitTypeLoc(TypeLoc T) {
    if (!main(T.getBeginLoc()))
      return true;
    const auto *D = T.getType()->getAsCXXRecordDecl();
    if (!D || !SeenTypes.insert(identity(D)).second)
      return true;
    llvm::json::Object O = describeDecl(AST, D);
    O.erase("text"); // 不把整个大型库类体混入调用清单。
    if (const auto *Def = D->getDefinition()) {
      O["aggregate"] = Def->isAggregate();
      O["polymorphic"] = Def->isPolymorphic();
      O["trivial_destructor"] = Def->hasTrivialDestructor();
      O["base_count"] = Def->getNumBases();
      unsigned Fields = 0;
      for (const auto *Field : Def->fields()) {
        (void)Field;
        ++Fields;
      }
      O["field_count"] = Fields;
      llvm::json::Array Bases;
      for (const auto &Base : Def->bases())
        Bases.push_back(Base.getType().getCanonicalType().getAsString());
      O["bases"] = std::move(Bases);
    }
    Result.Types.push_back(std::move(O));
    return true;
  }
  bool VisitCXXMethodDecl(CXXMethodDecl *D) {
    if (!main(D->getLocation()))
      return true;
    for (const auto *Base : D->overridden_methods())
      Result.Overrides.push_back(
          llvm::json::Object{{"derived", describeDecl(AST, D)},
                             {"base", describeDecl(AST, Base)}});
    return true;
  }
};
} // namespace
void collectInventory(ASTContext &AST, ProbeResult &Result) {
  Inventory Visitor(AST, Result);
  Visitor.TraverseDecl(AST.getTranslationUnitDecl());
}
} // namespace buddy::header_pilot
