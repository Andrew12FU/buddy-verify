// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_CONVERSION_LLVMCOMMON_CONVERSIONTARGET_H
#define BUDDY_MINI_MLIR_CONVERSION_LLVMCOMMON_CONVERSIONTARGET_H
#include "mlir/Transforms/DialectConversion.h"

namespace mlir {
class LLVMConversionTarget : public ConversionTarget {
public:
  explicit LLVMConversionTarget(MLIRContext &ctx);
};
} // namespace mlir
#endif
