// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_DIALECT_MEMREF_IR_MEMREF_H
#define BUDDY_MINI_MLIR_DIALECT_MEMREF_IR_MEMREF_H
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Builders.h"

namespace mlir {
namespace memref {
class MemRefDialect;
}
} // namespace mlir
namespace mlir { namespace memref {
 









 





} }

namespace mlir { namespace memref {
 






} }
namespace mlir { namespace memref {
 






} }
namespace mlir { namespace memref {
 






} }
namespace mlir { namespace memref {
 





} }
namespace mlir { namespace memref {
 


} }
namespace mlir { namespace memref {
class DimOp : public ::mlir::Op<DimOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::IndexType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::OpAsmOpInterface::Trait, ::mlir::OpTrait::MemRefsNormalizable, ::mlir::ConditionallySpeculatable::Trait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ShapedDimOpInterface::Trait, ::mlir::InferIntRangeInterface::Trait, ::mlir::InferTypeOpInterface::Trait> {
public:
 using Op::Op;
 static DimOp create(OpBuilder &,Location,Value,int64_t);
};
} }

#endif
