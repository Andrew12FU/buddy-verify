// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_MINI_SLICE_H
#define BUDDY_FRAMAC_MINI_SLICE_H
#include <string>
namespace clang::tooling {
class CompilationDatabase;
}
namespace buddy::framac {
/// Compute declaration/member deletions, without generating new C++
/// definitions.
/// @param Database Compilation flags selecting only the shared MiniMLIR
/// library.
/// @param Input Rewritten pass whose references seed the dependency closure.
/// @param LibraryRoot Absolute canonical include directory; declarations
/// outside it
///        cannot be deleted or imported by this operation.
/// @param Report New JSON file containing retained reasons and byte ranges.
/// @return 0 on success, 1 on parsing errors, 2 on invalid paths/output errors.
int sliceMiniLibrary(const clang::tooling::CompilationDatabase &Database,
                     const std::string &Input, const std::string &LibraryRoot,
                     const std::string &Report);
} // namespace buddy::framac
#endif
