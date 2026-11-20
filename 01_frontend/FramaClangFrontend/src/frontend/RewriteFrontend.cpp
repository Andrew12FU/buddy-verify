// SPDX-License-Identifier: Apache-2.0
/// @file 命令行入口。审查规则请转到 src/rules/*Rule.cpp，不要在这里新增 AST
/// 业务逻辑。
#include "frontend/FrontendOptions.h"
#include "frontend/RewritePipeline.h"
#include "support/FrontendOutput.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang::tooling;
using namespace buddy::framac;
namespace {
// 命令行变量只存在于入口；转换成普通 Options 后传给各层。
static llvm::cl::OptionCategory Category("Buddy Frama-Clang frontend");
static llvm::cl::opt<std::string> Output("output", llvm::cl::Required,
                                         llvm::cl::cat(Category));
static llvm::cl::opt<std::string> Report("report", llvm::cl::Required,
                                         llvm::cl::cat(Category));
static llvm::cl::opt<std::string>
    Rules("rules", llvm::cl::init("namespace,while,capture,guard"),
          llvm::cl::desc("Comma-separated compatibility rules"),
          llvm::cl::cat(Category));
static llvm::cl::opt<std::string> Profile(
    "profile", llvm::cl::init("reshape"),
    llvm::cl::desc(
        "Audited pass profile (see experiment/profiles.py for all supported passes)"),
    llvm::cl::cat(Category));
static llvm::cl::opt<std::string> NamespacePrefix("namespace-prefix",
                                                  llvm::cl::init(""),
                                                  llvm::cl::cat(Category));
static llvm::cl::opt<std::string> InterfaceHeader(
    "interface-header", llvm::cl::init(""),
    llvm::cl::desc("Optional include substitution AFTER parsing real headers"),
    llvm::cl::cat(Category));

} // namespace

/// 命令行外壳：解析参数 → 校验路径 → 规划 → 合并 → 报告/源码落盘。
/// @param argc 操作系统提供的参数个数。
/// @param argv 参数数组，-- 后参数交给 Clang。
/// @return 0=计划接受且两个产物写入成功；1=解析或规则拒绝；2=参数/I/O 错误。
/// @note C++ 返回 0 不代表 Frama-Clang 转换通过；该阶段由 Python 单独验证。
int main(int argc, const char **argv) {
  auto Parser = CommonOptionsParser::create(argc, argv, Category);
  if (!Parser) {
    llvm::errs() << llvm::toString(Parser.takeError()) << '\n';
    return 2;
  }
  FrontendOptions Options;
  Options.Profile = Profile;
  Options.NamespacePrefix = NamespacePrefix;
  Options.InterfaceHeader = InterfaceHeader;
  if (!isSupportedProfile(Options.Profile)) {
    llvm::errs() << "Unknown profile: " << Profile << '\n';
    return 2;
  }
  const auto &Sources = Parser->getSourcePathList();
  if (Sources.size() != 1 || samePath(Sources.front(), Output) ||
      samePath(Sources.front(), Report) || samePath(Output, Report)) {
    llvm::errs() << "Use one input and distinct output/report paths\n";
    return 2;
  }
  if (llvm::sys::fs::exists(Output) || llvm::sys::fs::exists(Report)) {
    llvm::errs() << "Output/report already exists; choose fresh paths\n";
    return 2;
  }
  if (InterfaceHeader.find_first_of("\"\n\r") != std::string::npos) {
    llvm::errs() << "Invalid interface include name\n";
    return 2;
  }
  llvm::SmallVector<llvm::StringRef> RuleList;
  llvm::StringRef(Rules).split(RuleList, ',');
  for (auto Rule : RuleList) {
    if (!isKnownRule(Rule)) {
      llvm::errs() << "Unknown rule: " << Rule << '\n';
      return 2;
    }
    Options.Rules.insert(Rule.str());
  }

  RewritePlan Plan;
  Plan.Input = Sources.front();
  int ParseStatus =
      parseAndPlan(Parser->getCompilations(), Plan.Input, Options, Plan);
  if (ParseStatus)
    Plan.reject("Clang parsing failed; no rewritten source emitted");
  bool Accepted = Plan.finish();
  // 先写可审查的计划（包括拒绝原因），只有内存计划接受后才写源码。
  // 报告的 status 描述计划状态；文件写入失败仍会返回 2，不冒充完成转换。
  if (!writeFile(Report, serializeReport(Plan, Options, Output, Accepted)))
    return 2;
  if (!Accepted)
    return 1;
  if (!writeFile(Output, Plan.Rewritten))
    return 2;
  llvm::outs() << "Applied " << Plan.Edits.size() << " source edits\n";
  return 0;
}
