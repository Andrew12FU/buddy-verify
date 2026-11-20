// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_DIALECT_TENSOR_IR_TENSOR_H
#define BUDDY_MINI_MLIR_DIALECT_TENSOR_IR_TENSOR_H
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir {
namespace tensor {
class EmptyOp
    : public Op<EmptyOp, OpTrait::OneTypedResult<RankedTensorType>::Impl> {
public:
  using Op::Op;
  static EmptyOp create(OpBuilder &builder, Location location,
                        ArrayRef<int64_t> staticShape, Type elementType,
                        Attribute encoding = {});
};
} // namespace tensor
} // namespace mlir
#endif
