// SPDX-License-Identifier: Apache-2.0
#include "analysis/Collect.h"
#include "generation/Emit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Signals.h"
using namespace clang;
namespace {
llvm::cl::OptionCategory
    category("ExtractMiniMLIR: real AST to coarse skeleton");
llvm::cl::opt<std::string> output("output-dir", llvm::cl::Required,
                                  llvm::cl::cat(category));
bool emitted = false;
llvm::json::Array macros;
/// Observe directly spelled main-file macro invocations only. No macro source
/// is copied into the library; missing macro support remains a separate
/// blocker.
class Macros : public PPCallbacks {
  SourceManager &sm;

public:
  explicit Macros(SourceManager &s) : sm(s) {}
  void MacroExpands(const Token &tok, const MacroDefinition &,
                    SourceRange range, const MacroArgs *) override {
    if (!range.getBegin().isFileID() ||
        !sm.isWrittenInMainFile(range.getBegin()))
      return;
    macros.push_back(llvm::json::Object{
        {"category", "input_macro"},
        {"id", "macro:" + sm.getFilename(range.getBegin()).str() + ":" +
                   std::to_string(sm.getSpellingLineNumber(range.getBegin())) +
                   ":" + tok.getIdentifierInfo()->getName().str()},
        {"name", tok.getIdentifierInfo()->getName()},
        {"use_line", sm.getSpellingLineNumber(range.getBegin())},
        {"reason",
         "Main-file macro invocation requires a separate "
         "expansion/reconstruction rule; macro definition not copied."}});
  }
};
class Consumer : public ASTConsumer {
  void HandleTranslationUnit(ASTContext &a) override {
    if (a.getDiagnostics().hasErrorOccurred())
      return;
    buddy::extract::Registry r(a);
    buddy::extract::collect(r);
    for (auto &v : macros)
      r.unsupported.push_back(v);
    emitted = buddy::extract::emit(r, output);
  }
};
class Action : public ASTFrontendAction {
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci,
                                                 llvm::StringRef) override {
    ci.getPreprocessor().addPPCallbacks(
        std::make_unique<Macros>(ci.getSourceManager()));
    return std::make_unique<Consumer>();
  }
};
} // namespace
/// One TU, read-only AST input, caller-owned current output. 0 means AST parsed
/// and output written, not that the candidate compiled; 1=parse,
/// 2=I/O/arguments.
int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  auto p = tooling::CommonOptionsParser::create(argc, argv, category);
  if (!p) {
    llvm::errs() << llvm::toString(p.takeError());
    return 2;
  }
  if (p->getSourcePathList().size() != 1)
    return 2;
  tooling::ClangTool tool(p->getCompilations(), p->getSourcePathList());
  int status = tool.run(tooling::newFrontendActionFactory<Action>().get());
  return status ? 1 : emitted ? 0 : 2;
}
