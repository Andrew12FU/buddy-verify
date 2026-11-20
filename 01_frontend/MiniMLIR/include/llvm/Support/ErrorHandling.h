// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_LLVM_SUPPORT_ERRORHANDLING_H
#define BUDDY_MINI_LLVM_SUPPORT_ERRORHANDLING_H
#include "mlir/Support/LLVM.h"
namespace llvm { void llvm_unreachable_internal(const char *,const char *,unsigned); }
#define llvm_unreachable(msg) ::llvm::llvm_unreachable_internal(msg,__FILE__,__LINE__)
#endif
