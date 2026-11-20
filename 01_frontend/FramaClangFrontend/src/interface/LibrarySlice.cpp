// SPDX-License-Identifier: Apache-2.0
#include "interface/Analysis.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"
using namespace clang;
namespace buddy::header_pilot {
namespace {
/// 从真实短函数体提取“调用/构造/字段引用”边；不分析任意间接派发、宏或类型闭包。
class Dependencies : public RecursiveASTVisitor<Dependencies> {
public:
  llvm::json::Array Edges;
  bool VisitCallExpr(CallExpr *E) {
    if (auto *D = E->getDirectCallee())
      Edges.push_back(identity(D));
    return true;
  }
  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    Edges.push_back(identity(E->getConstructor()));
    return true;
  }
  bool VisitMemberExpr(MemberExpr *E) {
    if (isa<FieldDecl>(E->getMemberDecl()))
      Edges.push_back(identity(E->getMemberDecl()));
    return true;
  }
};

/// @param D 函数或模板函数；@return 原文、仅声明文本、访问性和短函数依赖。
/// 构造初始化列表通过 AST 的右括号位置截断；当前两个类均无 ctor requires。
llvm::json::Object member(ASTContext &AST, const Decl *D) {
  auto O = describeDecl(AST, D);
  const auto *Template = dyn_cast<FunctionTemplateDecl>(D);
  const auto *F =
      Template ? Template->getTemplatedDecl() : dyn_cast<FunctionDecl>(D);
  O["access"] = D->getAccess() == AS_private ? "private" : "public";
  if (!F)
    return O;
  O["id"] = identity(F); // 依赖边指向实际函数，模板壳另存完整原文。
  O["signature"] = F->getType().getAsString();
  llvm::json::Array Parameters;
  for (const auto *P : F->parameters())
    Parameters.push_back(P->getType().getAsString());
  O["parameter_types"] = std::move(Parameters);
  O["function_text"] = describeDecl(AST, F).getString("text")->str();
  O["template"] = Template != nullptr;
  O["constructor"] = isa<CXXConstructorDecl>(F);
  auto *Body = F->getBody();
  std::string Prototype = O.getString("text")->str();
  if (Body) {
    auto &SM = AST.getSourceManager();
    auto End = Body->getBeginLoc();
    if (isa<CXXConstructorDecl>(F)) {
      auto T =
          F->getTypeSourceInfo()->getTypeLoc().getAs<FunctionProtoTypeLoc>();
      End = Lexer::getLocForEndOfToken(T.getRParenLoc(), 0, SM,
                                       AST.getLangOpts());
    }
    if (!D->getBeginLoc().isMacroID() && End.isValid())
      Prototype = Lexer::getSourceText(
                      CharSourceRange::getCharRange(D->getBeginLoc(), End), SM,
                      AST.getLangOpts())
                      .str();
    Dependencies Visitor;
    Visitor.TraverseStmt(const_cast<Stmt *>(Body));
    if (const auto *Ctor = dyn_cast<CXXConstructorDecl>(F))
      for (const auto *Init : Ctor->inits()) {
        if (Init->isMemberInitializer())
          Visitor.Edges.push_back(identity(Init->getMember()));
        Visitor.TraverseStmt(Init->getInit());
      }
    O["dependencies"] = std::move(Visitor.Edges);
  }
  O["declaration_text"] = Prototype + ";";
  return O;
}

/// 按命名空间查找 primary record，不递归遍历所有库函数体/实例化类。
void scan(DeclContext *Context, ASTContext &AST, ProbeResult &Result) {
  for (auto *D : Context->decls()) {
    if (auto *NS = dyn_cast<NamespaceDecl>(D)) {
      scan(NS, AST, Result);
      continue;
    }
    CXXRecordDecl *Record = dyn_cast<CXXRecordDecl>(D);
    if (auto *T = dyn_cast<ClassTemplateDecl>(D))
      Record = T->getTemplatedDecl();
    if (Record && Record->isThisDeclarationADefinition() &&
        (Record->getQualifiedNameAsString() == "llvm::LogicalResult" ||
         Record->getQualifiedNameAsString() == "llvm::ArrayRef") &&
        !isa<ClassTemplateSpecializationDecl>(Record)) {
      auto Object = describeDecl(AST, Record);
      llvm::json::Array Members;
      for (const auto *M : Record->decls())
        if (!M->isImplicit() && !isa<AccessSpecDecl>(M))
          Members.push_back(member(AST, M));
      Object["members"] = std::move(Members);
      Result.Library[Record->getNameAsString()] = std::move(Object);
    }
    const auto *F = dyn_cast<FunctionDecl>(D);
    if (F && !F->getDescribedFunctionTemplate() &&
        F->doesThisDeclarationHaveABody() &&
        (F->getQualifiedNameAsString() == "llvm::failure" ||
         F->getQualifiedNameAsString() == "llvm::failed"))
      Result.Library[F->getNameAsString()] = member(AST, F);
  }
}
} // namespace
void collectLibrarySlices(ASTContext &AST, ProbeResult &Result) {
  scan(AST.getTranslationUnitDecl(), AST, Result);
}
} // namespace buddy::header_pilot
