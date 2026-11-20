// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_CONVERSION_LLVMCOMMON_TYPECONVERTER_H
#define BUDDY_MINI_MLIR_CONVERSION_LLVMCOMMON_TYPECONVERTER_H
#include "mlir/Transforms/DialectConversion.h"

namespace mlir {
class DataLayoutAnalysis;
class LowerToLLVMOptions;
class LLVMTypeConverter : public TypeConverter {
public:
  LLVMTypeConverter(MLIRContext *ctx,
                    const DataLayoutAnalysis *analysis = nullptr);
  LLVMTypeConverter(MLIRContext *ctx, const LowerToLLVMOptions &options,
                    const DataLayoutAnalysis *analysis = nullptr);
};
} // namespace mlir
#endif
