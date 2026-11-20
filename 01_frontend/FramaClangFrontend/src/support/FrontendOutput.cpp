// SPDX-License-Identifier: Apache-2.0
#include "support/FrontendOutput.h"
#include "clang/Basic/Version.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
namespace buddy::framac {
bool writeFile(llvm::StringRef Path, llvm::StringRef Contents) {
  std::error_code EC;
  llvm::raw_fd_ostream Stream(Path, EC);
  if (EC) {
    llvm::errs() << "Cannot write " << Path << ": " << EC.message() << '\n';
    return false;
  }
  Stream << Contents;
  Stream.close();
  return !Stream.has_error();
}

bool samePath(llvm::StringRef A, llvm::StringRef B) {
  llvm::SmallString<256> RA, RB;
  // Resolve existing symlinks, then compare normalized absolute paths for new
  // files as well. Outputs must never overwrite the input or each other.
  auto Resolve = [](llvm::StringRef Path, llvm::SmallString<256> &R) {
    if (llvm::sys::fs::real_path(Path, R)) {
      R = Path;
      llvm::sys::fs::make_absolute(R);
      llvm::sys::path::remove_dots(R, true);
    }
  };
  Resolve(A, RA);
  Resolve(B, RB);
  bool Equivalent = false;
  llvm::sys::fs::equivalent(A, B, Equivalent);
  return Equivalent || RA == RB;
}

std::string serializeReport(const RewritePlan &Plan,
                            const FrontendOptions &Options,
                            llvm::StringRef OutputPath, bool Accepted) {
  llvm::json::Array Edits, Rejections, Enabled;
  for (const auto &E : Plan.Edits)
    Edits.push_back(
        llvm::json::Object{{"rule", E.Rule},
                           {"offset", E.Offset},
                           {"length", E.Length},
                           {"insertion_order", E.InsertionOrder},
                           {"line", E.Line},
                           {"column", E.Column},
                           {"before", E.Before},
                           {"after", E.After},
                           {"reason", E.Reason},
                           {"dependency_change", E.DependencyChange}});
  for (const auto &R : Plan.Rejections)
    Rejections.push_back(R);
  for (const auto &R : Options.Rules)
    Enabled.push_back(R);
  llvm::json::Object Result{
      {"schema_version", 1},
      {"input", Plan.Input},
      {"output", OutputPath.str()},
      {"clang_version", getClangFullVersion()},
      {"profile", Options.Profile},
      {"parsed", Plan.Parsed},
      {"status", Accepted ? "rewritten" : "rejected"},
      {"rules", std::move(Enabled)},
      {"edits", std::move(Edits)},
      {"rejections", std::move(Rejections)},
      {"scope",
       "Audited pass compatibility rules; no general equivalence claim"}};
  std::string JSON;
  llvm::raw_string_ostream Stream(JSON);
  Stream << llvm::formatv("{0:2}\n", llvm::json::Value(std::move(Result)));
  return Stream.str();
}
} // namespace buddy::framac
