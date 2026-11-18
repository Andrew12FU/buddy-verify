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
class ExtractStridedMetadataOp : public ::mlir::Op<ExtractStridedMetadataOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::AtLeastNResults<2>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::OpInvariants, ::mlir::OpAsmOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ViewLikeOpInterface::Trait, ::mlir::InferTypeOpInterface::Trait, ::mlir::OpTrait::InferTypeOpAdaptor> {
public:
  using Op::Op;
  static ExtractStridedMetadataOp create(OpBuilder &builder, Location location, Value source);
   
  ResultRange getSizes();
  ResultRange getStrides();
  TypedValue<IndexType> getOffset();
  TypedValue<MemRefType> getBaseBuffer();
};
class ReinterpretCastOp : public ::mlir::Op<ReinterpretCastOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::MemRefType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<1>::Impl, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::OpTrait::MemRefsNormalizable, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::OffsetSizeAndStrideOpInterface::Trait, ::mlir::ViewLikeOpInterface::Trait> {
public:
  using Op::Op;
   
  static ReinterpretCastOp create(OpBuilder &builder, Location location, MemRefType resultType, Value source, OpFoldResult offset, ArrayRef<OpFoldResult> sizes, ArrayRef<OpFoldResult> strides);
};
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
 




} }

#endif
