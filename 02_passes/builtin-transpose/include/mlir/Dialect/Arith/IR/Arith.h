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
 
class ConstantOp : public Op<ConstantOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static ConstantOp create(OpBuilder &, Location, TypedAttr);
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


namespace mlir { namespace arith { enum class AtomicRMWKind : uint64_t {
  addf = 0,
  addi = 1,
  andi = 2,
  assign = 3,
  maximumf = 4,
  maxnumf = 5,
  maxs = 6,
  maxu = 7,
  minimumf = 8,
  minnumf = 9,
  mins = 10,
  minu = 11,
  mulf = 12,
  muli = 13,
  ori = 14,
  xori = 15,
}; } }

#endif
