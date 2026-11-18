// SPDX-License-Identifier: Apache-2.0
/// @file Shared MiniMLIR member slicer command line. See MiniSlice.cpp for
/// rules.
#include "interface/MiniSlice.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

static llvm::cl::OptionCategory Category("MiniMLIR declaration extraction");
static llvm::cl::opt<std::string> Root("library-root", llvm::cl::Required,
                                       llvm::cl::cat(Category));
static llvm::cl::opt<std::string> Report("report", llvm::cl::Required,
                                         llvm::cl::cat(Category));

/// Parse one pass against the canonical mini and write an immutable slicing
/// plan.
/// @return 0 on a parsed plan, 1 on diagnostics, 2 on invalid arguments or
/// output.
int main(int argc, const char **argv) {
  auto Parser =
      clang::tooling::CommonOptionsParser::create(argc, argv, Category);
  if (!Parser) {
    llvm::errs() << llvm::toString(Parser.takeError()) << '\n';
    return 2;
  }
  if (Parser->getSourcePathList().size() != 1)
    return 2;
  return buddy::framac::sliceMiniLibrary(Parser->getCompilations(),
                                         Parser->getSourcePathList().front(),
                                         Root, Report);
}
