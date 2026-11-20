// SPDX-License-Identifier: Apache-2.0
#include "interface/Analysis.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Basic/Version.h"
#include "llvm/Support/FormatVariadic.h"
using namespace clang;
namespace buddy::header_pilot {
namespace {
/// 记录主文件 include 与 TypeID 等宏；额外保存 ArrayRef 的寿命属性宏展开。
class PreprocessorFacts : public PPCallbacks {
  Preprocessor &PP;
  ProbeResult &Result;

public:
  PreprocessorFacts(Preprocessor &PP, ProbeResult &Result)
      : PP(PP), Result(Result) {}
  void InclusionDirective(SourceLocation Hash, const Token &,
                          llvm::StringRef Name, bool, CharSourceRange,
                          OptionalFileEntryRef File, llvm::StringRef,
                          llvm::StringRef, const Module *,
#if CLANG_VERSION_MAJOR >= 19
                          bool,
#endif
                          SrcMgr::CharacteristicKind) override {
    auto &SM = PP.getSourceManager();
    if (SM.isWrittenInMainFile(Hash))
      Result.Includes.push_back(
          llvm::json::Object{{"name", Name},
                             {"resolved", File ? File->getName().str() : ""},
                             {"line", SM.getSpellingLineNumber(Hash)}});
  }
  void MacroExpands(const Token &Name, const MacroDefinition &Definition,
                    SourceRange Range, const MacroArgs *) override {
    auto &SM = PP.getSourceManager();
    auto Text = PP.getSpelling(Name);
    if (!SM.isWrittenInMainFile(SM.getExpansionLoc(Range.getBegin())) &&
        Text != "LLVM_LIFETIME_BOUND")
      return;
    const auto *Info = Definition.getMacroInfo();
    std::string Replacement;
    if (Info)
      for (const auto &Token : Info->tokens()) {
        if (!Replacement.empty())
          Replacement += " ";
        Replacement += PP.getSpelling(Token);
      }
    Result.Macros.push_back(llvm::json::Object{
        {"name", Text},
        {"replacement", Replacement},
        {"function_like", Info && Info->isFunctionLike()},
        {"main_file",
         SM.isWrittenInMainFile(SM.getExpansionLoc(Range.getBegin()))}});
  }
};
class Consumer : public ASTConsumer {
  ProbeResult &Result;

public:
  explicit Consumer(ProbeResult &Result) : Result(Result) {}
  void HandleTranslationUnit(ASTContext &AST) override {
    if (AST.getDiagnostics().hasErrorOccurred())
      return;
    collectInventory(AST, Result);
    collectLibrarySlices(AST, Result);
    Result.Parsed = true;
  }
};
class Action : public ASTFrontendAction {
  ProbeResult &Result;

public:
  explicit Action(ProbeResult &Result) : Result(Result) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    CI.getPreprocessor().addPPCallbacks(
        std::make_unique<PreprocessorFacts>(CI.getPreprocessor(), Result));
    return std::make_unique<Consumer>(Result);
  }
};
class Factory : public tooling::FrontendActionFactory {
  ProbeResult &Result;

public:
  explicit Factory(ProbeResult &Result) : Result(Result) {}
  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<Action>(Result);
  }
};
} // namespace
std::unique_ptr<tooling::FrontendActionFactory>
makeProbeFactory(ProbeResult &Result) {
  return std::make_unique<Factory>(Result);
}
std::string serialize(ProbeResult &R, const std::string &Input) {
  llvm::json::Object O{
      {"schema_version", 1},
      {"clang_version", getClangFullVersion()},
      {"input", Input},
      {"parsed", R.Parsed},
      {"calls", std::move(R.Calls)},
      {"types", std::move(R.Types)},
      {"overrides", std::move(R.Overrides)},
      {"includes", std::move(R.Includes)},
      {"macros", std::move(R.Macros)},
      {"library", std::move(R.Library)},
      {"scope",
       "Bounded extraction pilot; no full MLIR header generation or proof"}};
  std::string Text;
  llvm::raw_string_ostream Stream(Text);
  Stream << llvm::formatv("{0:2}\n", llvm::json::Value(std::move(O)));
  return Stream.str();
}
} // namespace buddy::header_pilot
