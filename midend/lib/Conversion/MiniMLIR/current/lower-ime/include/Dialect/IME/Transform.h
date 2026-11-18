// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_DIALECT_IME_TRANSFORM_H
#define BUDDY_MINI_DIALECT_IME_TRANSFORM_H
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"

namespace mlir {
void configureIMELegalizeForExportTarget(LLVMConversionTarget &target);
void populateIMELegalizeForLLVMExportPatterns(LLVMTypeConverter &converter,
                                              RewritePatternSet &patterns);
} // namespace mlir
#endif
