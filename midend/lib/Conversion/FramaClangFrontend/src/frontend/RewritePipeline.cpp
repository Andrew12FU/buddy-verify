// SPDX-License-Identifier: Apache-2.0
#include "frontend/RewritePipeline.h"
#include "rules/CaptureRule.h"
#include "rules/GuardRule.h"
#include "rules/IfConditionRule.h"
#include "rules/NamespaceRule.h"
#include "rules/ValueRangeRule.h"
#include "rules/WhileRule.h"
#include "rules/RangeForRule.h"
#include "rules/ConstructorRule.h"
#include "rules/ConstLocalRule.h"
#include "rules/InitializerListRule.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Basic/Version.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::tooling;
namespace buddy::framac {
namespace {
/// 预处理适配器：仅登记主文件中白名单 include 的输出修改。
/// Clang 仍读取真实 include；不会在解析途中切换到 mini。
class IncludeCollector : public PPCallbacks {
  SourceManager &SM;
  RewritePlan &Plan;
  const FrontendOptions &Options;
  bool First = true; // 多个原 include 合并为一次 mini include。
public:
  /// @param SM 当前翻译单元的源码管理器，仅借用。
  /// @param Plan 输出候选编辑的共享计划。
  /// @param Options 外层持有的只读配置。
  IncludeCollector(SourceManager &SM, RewritePlan &Plan,
                   const FrontendOptions &Options)
      : SM(SM), Plan(Plan), Options(Options) {}

  /// Clang 每遇到一条实际处理的 include 调用一次。
  /// @param Hash 指令的 # 位置，用于计算整个指令行的原始字节范围。
  /// @param Name include 文件名；其余回调参数不参与当前规则。
  /// @return 无；遇到有效白名单时向计划追加替换/删除，其他情况跳过。
  void InclusionDirective(SourceLocation Hash, const Token &,
                          llvm::StringRef Name, bool, CharSourceRange,
                          OptionalFileEntryRef, llvm::StringRef,
                          llvm::StringRef, const Module *,
#if CLANG_VERSION_MAJOR >= 19
                          bool,
#endif
                          SrcMgr::CharacteristicKind) override {
    if (Options.InterfaceHeader.empty() || !SM.isWrittenInMainFile(Hash) ||
        !isInterfaceInclude(Options.Profile, Name))
      return;
    auto Buffer = SM.getBufferData(SM.getMainFileID());
    unsigned Begin = SM.getFileOffset(Hash);
    size_t End = Buffer.find('\n', Begin);
    End = End == llvm::StringRef::npos ? Buffer.size() : End + 1;
    auto Replacement =
        First ? "#include \"" + Options.InterfaceHeader + "\"\n" : "";
    First = false;
    Plan.add(
        SM, Hash,
        SM.getLocForStartOfFile(SM.getMainFileID()).getLocWithOffset(End),
        "interface-substitution", Replacement,
        "Explicit experiment setup; real dependency headers were parsed first",
        "Uses an existing audited interface model; no header extraction");
  }
};

/// 第二遍 AST 遍历只做分发。每个 Visit* 返回 true 表示“继续遍历”，
/// 不是“该节点已经改写成功”；真正结果在 Plan.Edits / Plan.Rejections 中。
/// CRTP、getDerived() 和默认遍历的间接递归说明见 AstSupport.cpp 的
/// MainFileVisitor。
class RuleDispatcher : public RecursiveASTVisitor<RuleDispatcher> {
  RewriteContext &R;
  NamespaceRule Namespaces;

public:
  /// @param R 已完成主文件清单收集的共享上下文。
  explicit RuleDispatcher(RewriteContext &R) : R(R), Namespaces(R) {}
  /// @param D 待遍历声明；头文件声明跳过，主文件声明按默认递归策略访问。
  /// @return true 继续，递归返回 false 时才表示遍历中断。
  bool TraverseDecl(Decl *D) {
    if (D && D->getLocation().isValid() && !R.isMainFile(D->getLocation()))
      return true;
    // 委托基类分发；遍历到子声明后，getDerived().TraverseDecl(Child)
    // 再进入本方法。
    return RecursiveASTVisitor::TraverseDecl(D);
  }
  /// @param D 当前命名空间声明。
  /// @return 总为 true，规则结果写入计划。
  bool VisitNamespaceDecl(NamespaceDecl *D) {
    Namespaces.planDeclaration(D);
    return true;
  }
  /// @param T 当前原文类类型位置。
  /// @return 总为 true，继续访问其他节点。
  bool VisitRecordTypeLoc(RecordTypeLoc T) {
    Namespaces.planTypeReference(T);
    return true;
  }
  /// @param W 当前 while。
  /// @return 总为 true，拒绝不通过返回值传播。
  bool VisitUsingDecl(UsingDecl *D) {
    planConstructorRewrite(D, R);
    return true;
  }
  bool VisitCXXForRangeStmt(CXXForRangeStmt *S) {
    planRangeForRewrite(S, R);
    return true;
  }
  bool VisitDeclStmt(DeclStmt *S) { planConstLocalRewrite(S,R); return true; }
  bool VisitStmt(Stmt *S) { planInitializerListRewrite(S,R); return true; }
  bool VisitWhileStmt(WhileStmt *W) {
    // 默认遍历找到 while 节点后回调到这里；W 是节点指针，R
    // 共享当前翻译单元上下文。
    planWhileRewrite(W, R);
    return true;
  }
  /// @param L 当前 lambda。
  /// @return 总为 true，具体规则可登记拒绝。
  bool VisitLambdaExpr(LambdaExpr *L) {
    planCaptureRewrite(L, R);
    return true;
  }
  /// @param S 当前 if。
  /// @return 总为 true，未匹配节点自然跳过。
  bool VisitIfStmt(IfStmt *S) {
    planIfConditionRewrite(S, R);
    planGuardRewrite(S, R);
    return true;
  }
  /// @param Call 调用节点；回调、列表转换及具体重载的判断属于规则层。
  /// @return true 继续遍历，规则只写计划。
  bool VisitCallExpr(CallExpr *Call) {
    planValueRangeRewrite(Call, R);
    return true;
  }
};

/// Clang 在原始翻译单元完整解析后调用的适配器。
class RewriteConsumer : public ASTConsumer {
  RewritePlan &Plan;
  const FrontendOptions &Options;

public:
  /// @param Plan 借用本次修改计划。
  /// @param Options 外层持有的只读配置。
  RewriteConsumer(RewritePlan &Plan, const FrontendOptions &Options)
      : Plan(Plan), Options(Options) {}
  /// @param AST 已解析的当前翻译单元。
  /// @return 无；有解析错误时不运行规则，否则先收集清单再规划修改。
  void HandleTranslationUnit(ASTContext &AST) override {
    if (AST.getDiagnostics().hasErrorOccurred())
      return;
    // 清单、上下文和分发器都在本次回调中使用；此时 Clang 仍持有有效的 AST。
    // 规则把结果保存成文本修改，不能把借用的 AST 节点指针留到 AST 销毁后使用。
    auto Inventory = collectMainFileInventory(AST);
    RewriteContext Context(AST, Plan, Options, Inventory);
    RuleDispatcher Dispatcher(Context);
    Dispatcher.TraverseDecl(AST.getTranslationUnitDecl());
    Plan.Parsed = true;
  }
};

/// 每个输入文件的前端动作；当前入口明确限制一次一个主文件。
class RewriteAction : public ASTFrontendAction {
  RewritePlan &Plan;
  const FrontendOptions &Options;

public:
  /// @param Plan 当前计划，由入口持有。
  /// @param Options 当前配置，生命周期覆盖整个 action。
  RewriteAction(RewritePlan &Plan, const FrontendOptions &Options)
      : Plan(Plan), Options(Options) {}
  /// @param CI Clang 持有的编译器实例，管理 SourceManager 和预处理器。
  /// @param UnusedInputName 输入名称由 Plan.Input 记录，当前回调无需再次使用。
  /// @return 交给 Clang 独占管理的 consumer；不转移 Plan/Options 的所有权。
  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI,
                    llvm::StringRef UnusedInputName) override {
    auto &SM = CI.getSourceManager();
    Plan.Original = SM.getBufferData(SM.getMainFileID()).str();
    CI.getPreprocessor().addPPCallbacks(
        std::make_unique<IncludeCollector>(SM, Plan, Options));
    return std::make_unique<RewriteConsumer>(Plan, Options);
  }
};

/// ClangTool 要求由工厂为每个输入创建独立 action。
class RewriteActionFactory : public FrontendActionFactory {
  RewritePlan &Plan;
  const FrontendOptions &Options;

public:
  /// @param Plan 所创建 action 共用的单文件计划。
  /// @param Options 单次执行的只读配置。
  RewriteActionFactory(RewritePlan &Plan, const FrontendOptions &Options)
      : Plan(Plan), Options(Options) {}
  /// @return 新 action 的独占所有权；Plan/Options 仍由 main 持有。
  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<RewriteAction>(Plan, Options);
  }
};
} // namespace

int parseAndPlan(CompilationDatabase &Compilations, const std::string &Input,
                 const FrontendOptions &Options, RewritePlan &Plan) {
  ClangTool Tool(Compilations, {Input});
  RewriteActionFactory Factory(Plan, Options);
  return Tool.run(&Factory);
}
} // namespace buddy::framac
