// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_DIALECT_MATH_IR_MATH_H
#define BUDDY_MINI_MLIR_DIALECT_MATH_IR_MATH_H
#include "mlir/Dialect/Arith/IR/Arith.h"

namespace mlir {
namespace math {
class ExpOp : public Op<ExpOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static ExpOp
  create(OpBuilder &, Location, Value,
         arith::FastMathFlags fastmath = arith::FastMathFlags::none);
};
} // namespace math
} // namespace mlir
#endif
