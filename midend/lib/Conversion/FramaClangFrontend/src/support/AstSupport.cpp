// SPDX-License-Identifier: Apache-2.0
#include "support/AstSupport.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"

using namespace clang;
namespace buddy::framac {
namespace {
// 这些 Visit* 返回 true 是继续遍历的约定，不表示某个改写成功。
// 这里使用 CRTP：将 MainFileVisitor 自身作为基类模板参数 Derived。
// 基类的 getDerived() 实现是 return *static_cast<Derived *>(this)：
// static_cast 是 C++ 内置转换语法，把基类 this 指针显式转回派生类指针，
// 再解引用返回原对象的引用，不创建或复制对象，也不进行运行时类型检查。
// 此处实际对象是 MainFileVisitor，因此满足该转换的前提。
//
// 基类通过 getDerived().Visit... / Traverse... 在编译期选择处理方法，
// 无须 virtual 分发，并给编译器提供内联机会：
//   提供了同名方法 -> 调用本类的方法，例如 TraverseDecl、VisitNamespaceDecl；
//   没有提供     -> 使用继承来的默认实现，例如 TraverseNamespaceDecl。
// 默认 Visit* 通常只返回 true，让我们只实现关心的回调。即使采用默认
// TraverseNamespaceDecl，其内部仍会通过 getDerived() 调用本类的回调。
// 若基类改用非虚的 this->VisitNamespaceDecl，则会选中基类版本，绕过本类回调。
// 实现可对照 llvm/clang/include/clang/AST/RecursiveASTVisitor.h 中的
// getDerived、TraverseDecl、DEF_TRAVERSE_DECL 和 TraverseDeclContextHelper。
class MainFileVisitor : public RecursiveASTVisitor<MainFileVisitor> {
public:
  // 借用 AST 的源码管理器：解释节点的 SourceLocation 属于哪个文件、宏展开点等。
  SourceManager &SM;
  std::set<std::string> Names;
  std::vector<NamespaceDecl *> Anonymous;
  // 初始化列表左边 SM 是成员，右边 SM 是参数；只绑定引用，不复制管理器。
  explicit MainFileVisitor(SourceManager &SM) : SM(SM) {}
  // Decl 是声明/定义节点的公共基类，如 NamespaceDecl、FunctionDecl、VarDecl；
  // WhileStmt 等语句属于另一类节点，由 TraverseStmt 一侧处理。
  bool TraverseDecl(Decl *D) {
    // 按宏展开位置判断：头文件中的普通声明跳过，在主文件展开生成的声明保留。
    // return true 在这里表示跳过当前声明及其子树，继续处理其他节点；false
    // 才中止。
    if (D && D->getLocation().isValid() &&
        !SM.isWrittenInMainFile(SM.getExpansionLoc(D->getLocation())))
      return true;
    // 必须限定调用基类版本。若写 TraverseDecl(D)，会反复以同一个 D 调用自身，
    // 通过上述检查后便无限递归，无法进入子节点。
    // 基类先按 D->getKind() 分发，真正的向下递归发生在后续辅助函数中，例如：
    //   TraverseDecl(namespace A)
    //     -> TraverseNamespaceDecl(A) -> TraverseDeclContextHelper(A)
    //     -> getDerived().TraverseDecl(Child) ->
    //     本方法(子声明)，再次检查文件位置。
    // 函数体则经 TraverseStmt -> CompoundStmt -> DeclStmt 找到局部 VarDecl，
    // 再回到 TraverseDecl；所以递归不只发生在同一个函数内。
    return RecursiveASTVisitor::TraverseDecl(D);
  }
  // 默认前序遍历命名空间时，同一个 D 依次经过 VisitDecl、VisitNamedDecl、
  // VisitNamespaceDecl。这是按类型继承层次触发回调，并非递归进入子节点。
  bool VisitNamedDecl(NamedDecl *D) {
    if (D->getIdentifier())
      Names.insert(D->getNameAsString());
    return true;
  }
  bool VisitNamespaceDecl(NamespaceDecl *D) {
    // 匿名空间没有标识符，不会被上面的 Names 收集；这里单独保存声明节点地址。
    if (D->isAnonymousNamespace())
      Anonymous.push_back(D);
    return true;
  }
};

class BodyFactsVisitor : public RecursiveASTVisitor<BodyFactsVisitor> {
public:
  bool HasJump = false, NestedLambda = false, Unevaluated = false;
  std::vector<DeclRefExpr *> References;
  bool VisitGotoStmt(GotoStmt *) {
    HasJump = true;
    return true;
  }
  bool VisitIndirectGotoStmt(IndirectGotoStmt *) {
    HasJump = true;
    return true;
  }
  bool VisitLabelStmt(LabelStmt *) {
    HasJump = true;
    return true;
  }
  bool VisitLambdaExpr(LambdaExpr *) {
    NestedLambda = true;
    return true;
  }
  bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *) {
    Unevaluated = true;
    return true;
  }
  bool VisitDecltypeTypeLoc(DecltypeTypeLoc) {
    Unevaluated = true;
    return true;
  }
  bool VisitCXXTypeidExpr(CXXTypeidExpr *) {
    Unevaluated = true;
    return true;
  }
  bool VisitDeclRefExpr(DeclRefExpr *E) {
    References.push_back(E);
    return true;
  }
};

} // namespace

RewriteContext::RewriteContext(ASTContext &AST, RewritePlan &Plan,
                               const FrontendOptions &Options,
                               MainFileInventory &Inventory)
    : AST(AST), SM(AST.getSourceManager()), Lang(AST.getLangOpts()), Plan(Plan),
      Options(Options), Inventory(Inventory) {}

MainFileInventory collectMainFileInventory(ASTContext &AST) {
  MainFileVisitor Visitor(AST.getSourceManager());
  // TranslationUnitDecl 是整个翻译单元的根，从这里进入上述间接递归。
  Visitor.TraverseDecl(AST.getTranslationUnitDecl());
  // move 转移容器中的字符串/指针，不转移 AST 节点的所有权或延长其生命周期。
  return {std::move(Visitor.Names), std::move(Visitor.Anonymous)};
}

BodyFacts inspectBody(Stmt *Body) {
  BodyFactsVisitor Visitor;
  Visitor.TraverseStmt(Body);
  return {Visitor.HasJump, Visitor.NestedLambda, Visitor.Unevaluated,
          std::move(Visitor.References)};
}

bool RewriteContext::isMainFile(SourceLocation L) const {
  return L.isValid() && SM.isWrittenInMainFile(SM.getExpansionLoc(L));
}
SourceLocation RewriteContext::tokenEnd(SourceLocation L) const {
  if (L.isInvalid() || L.isMacroID())
    return {};
  return Lexer::getLocForEndOfToken(L, 0, SM, Lang);
}
std::string RewriteContext::sourceText(SourceRange R) const {
  return Lexer::getSourceText(CharSourceRange::getTokenRange(R), SM, Lang)
      .str();
}
SourceLocation RewriteContext::statementEnd(const Stmt *S) const {
  auto E = tokenEnd(S->getEndLoc());
  if (auto T = Lexer::findNextToken(S->getEndLoc(), SM, Lang))
    if (T->is(tok::semi))
      E = tokenEnd(T->getLocation());
  return E;
}
std::string RewriteContext::indentation(SourceLocation L) const {
  unsigned Pos = SM.getFileOffset(L), Start = Pos;
  while (Start && Plan.Original[Start - 1] != '\n')
    --Start;
  std::string Prefix = Plan.Original.substr(Start, Pos - Start);
  if (Prefix.find_first_not_of(" \t") != std::string::npos)
    return "";
  return Prefix;
}
std::string RewriteContext::indentFollowingLines(llvm::StringRef Text) {
  std::string Result;
  for (size_t I = 0; I < Text.size(); ++I) {
    Result += Text[I];
    if (Text[I] == '\n' && I + 1 < Text.size() && Text[I + 1] != '\n')
      Result += "  ";
  }
  return Result;
}
const NamespaceDecl *
RewriteContext::anonymousOwner(const DeclContext *DC) const {
  for (; DC; DC = DC->getParent())
    if (const auto *NS = dyn_cast<NamespaceDecl>(DC))
      if (NS->isAnonymousNamespace())
        return NS->getCanonicalDecl();
  return nullptr;
}
bool RewriteContext::insideNamespace(SourceLocation L,
                                     const NamespaceDecl *Owner) const {
  unsigned Offset = SM.getFileOffset(L);
  for (const auto *NS : Inventory.Anonymous)
    if (NS->getCanonicalDecl() == Owner &&
        Offset >= SM.getFileOffset(NS->getBeginLoc()) &&
        Offset <= SM.getFileOffset(NS->getRBraceLoc()))
      return true;
  return false;
}
std::string RewriteContext::freshName(std::string Base) {
  std::string Name = Base;
  for (unsigned I = 1; Inventory.Names.count(Name); ++I)
    Name = Base + std::to_string(I);
  Inventory.Names.insert(Name);
  return Name;
}
bool RewriteContext::hasAddressOperator(const CXXRecordDecl *D) const {
  if (!D || !(D = D->getDefinition()))
    return true;
  for (const auto *M : D->methods())
    if (M->isOverloadedOperator() && M->getOverloadedOperator() == OO_Amp)
      return true;
  for (const auto *F : D->friends())
    if (const auto *Function =
            dyn_cast_or_null<FunctionDecl>(F->getFriendDecl()))
      if (Function->isOverloadedOperator() &&
          Function->getOverloadedOperator() == OO_Amp &&
          Function->getNumParams() == 1)
        return true;
  // 除成员/友元/基类外，也保守检查包围空间的一元取地址重载及模板。
  // 这是风险筛选，不是完整模板 ADL/重载可行性判定，不能泛化为安全证明。
  auto Operator = AST.DeclarationNames.getCXXOperatorName(OO_Amp);
  for (auto *DC = D->getDeclContext(); DC; DC = DC->getParent()) {
    if (!isa<NamespaceDecl>(DC) && !isa<TranslationUnitDecl>(DC))
      continue;
    for (const auto *Found : DC->lookup(Operator)) {
      const FunctionDecl *Function = dyn_cast<FunctionDecl>(Found);
      if (const auto *Template = dyn_cast<FunctionTemplateDecl>(Found))
        Function = Template->getTemplatedDecl();
      if (Function && Function->getNumParams() == 1)
        return true;
    }
  }
  for (const auto &B : D->bases())
    if (hasAddressOperator(B.getType()->getAsCXXRecordDecl()))
      return true;
  return false;
}

} // namespace buddy::framac
