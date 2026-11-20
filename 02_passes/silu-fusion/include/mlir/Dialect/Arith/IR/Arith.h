// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_DIALECT_ARITH_IR_ARITH_H
#define BUDDY_MINI_MLIR_DIALECT_ARITH_IR_ARITH_H
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir {
namespace arith {
class ArithDialect;
enum class FastMathFlags : unsigned { none = 0 };
class ConstantOp : public Op<ConstantOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static ConstantOp create(OpBuilder &, Location, TypedAttr);
};
class NegFOp : public Op<NegFOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static NegFOp create(OpBuilder &, Location, Value,
                       FastMathFlags fastmath = FastMathFlags::none);
};
class AddFOp : public Op<AddFOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static AddFOp create(OpBuilder &, Location, Value, Value,
                       FastMathFlags fastmath = FastMathFlags::none);
};
class DivFOp : public Op<DivFOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static DivFOp create(OpBuilder &, Location, Value, Value,
                       FastMathFlags fastmath = FastMathFlags::none);
};
class MulFOp : public Op<MulFOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static MulFOp create(OpBuilder &, Location, Value, Value,
                       FastMathFlags fastmath = FastMathFlags::none);
};
} // namespace arith
} // namespace mlir
namespace mlir { namespace arith {     } }
namespace mlir { namespace arith { namespace detail {   }
 
namespace detail {   }
} }
namespace mlir { namespace arith {
 




} }
namespace mlir { namespace arith {
 




} }
namespace mlir { namespace arith {
 




} }


namespace mlir { namespace arith {  
















 } }

#endif
