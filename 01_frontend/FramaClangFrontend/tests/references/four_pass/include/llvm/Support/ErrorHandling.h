#ifndef FC_LLVM_SUPPORT_ERRORHANDLING_H
#define FC_LLVM_SUPPORT_ERRORHANDLING_H

#include "llvm/ADT/StringRef.h"

namespace llvm {

/* 官方 23.x 有 report_fatal_error 的三个重载（const char 指针 / StringRef /
 * Twine，均带 bool gen_crash_diag，且已标 deprecated），另有
 * reportFatalInternalError / reportFatalUsageError。模型按"只声明用到的"
 * 保留前两个，并提供 llvm_unreachable 的展开目标。 */
void report_fatal_error(const char *reason, bool gen_crash_diag = true);
void report_fatal_error(StringRef reason, bool gen_crash_diag = true);

void llvm_unreachable_internal(const char *msg = 0, const char *file = 0,
                               unsigned line = 0);

} // namespace llvm

#define llvm_unreachable(msg)                                                  \
  ::llvm::llvm_unreachable_internal(msg, __FILE__, __LINE__)
#endif