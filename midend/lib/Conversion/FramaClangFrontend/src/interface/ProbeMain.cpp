// SPDX-License-Identifier: Apache-2.0
#include "interface/Probe.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang::tooling;
namespace {
llvm::cl::OptionCategory Category("Independent header extraction pilot");
llvm::cl::opt<std::string> Report("report", llvm::cl::Required,
                                  llvm::cl::cat(Category));
} // namespace
/// 单文件观察入口；0=解析并写报告成功，1=Clang 错误，2=参数/路径/输出错误。
/// 已存在报告不覆盖；不改输入、不生成生产 mini，也不调用主前端的二进制。
int main(int argc, const char **argv) {
  auto Parser = CommonOptionsParser::create(argc, argv, Category);
  if (!Parser) {
    llvm::errs() << llvm::toString(Parser.takeError()) << '\n';
    return 2;
  }
  if (Parser->getSourcePathList().size() != 1 ||
      llvm::sys::fs::exists(Report)) {
    llvm::errs() << "Use one input and a fresh report path\n";
    return 2;
  }
  buddy::header_pilot::ProbeResult Result;
  ClangTool Tool(Parser->getCompilations(), Parser->getSourcePathList());
  auto Factory = buddy::header_pilot::makeProbeFactory(Result);
  int Status = Tool.run(Factory.get());
  std::error_code Error;
  llvm::raw_fd_ostream Stream(Report, Error);
  if (Error) {
    llvm::errs() << Error.message() << '\n';
    return 2;
  }
  Stream << buddy::header_pilot::serialize(Result,
                                           Parser->getSourcePathList().front());
  Stream.close();
  return Stream.has_error() ? 2 : Status;
}
