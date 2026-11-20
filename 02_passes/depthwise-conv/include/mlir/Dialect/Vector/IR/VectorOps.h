// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations; algorithms require external specifications.
#pragma once
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
namespace mlir { namespace vector { class VectorDialect : public Dialect { public: explicit VectorDialect(MLIRContext *context); }; } }
namespace mlir { namespace vector {
class LoadOp : public ::mlir::Op<LoadOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<1>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::AlignmentAttrOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static LoadOp create(OpBuilder &,Location,VectorType,Value,ValueRange);
};
} }
namespace mlir { namespace vector {
class StoreOp : public ::mlir::Op<StoreOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::AlignmentAttrOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static StoreOp create(OpBuilder &,Location,Value,Value,ValueRange);
};
} }
namespace mlir { namespace vector {
class BroadcastOp : public ::mlir::Op<BroadcastOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::InferIntRangeInterface::Trait> {
public:
 using Op::Op;
 static BroadcastOp create(OpBuilder &,Location,VectorType,Value);
};
} }
namespace mlir { namespace vector {
class CreateMaskOp : public ::mlir::Op<CreateMaskOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait> {
public:
 using Op::Op;
 static CreateMaskOp create(OpBuilder &,Location,Type,ValueRange); static CreateMaskOp create(OpBuilder &,Location,VectorType,ArrayRef<OpFoldResult>);
};
} }
namespace mlir { namespace vector {
class MaskedLoadOp : public ::mlir::Op<MaskedLoadOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<3>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::AlignmentAttrOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static MaskedLoadOp create(OpBuilder &,Location,Type,Value,ValueRange,Value,Value);
};
} }
namespace mlir { namespace vector {
class MaskedStoreOp : public ::mlir::Op<MaskedStoreOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<3>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::AlignmentAttrOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static MaskedStoreOp create(OpBuilder &,Location,Value,ValueRange,Value,Value);
};
} }
namespace mlir { namespace vector {
class FMAOp : public ::mlir::Op<FMAOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<3>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::OpTrait::Elementwise, ::mlir::OpTrait::Scalarizable, ::mlir::OpTrait::Vectorizable, ::mlir::OpTrait::Tensorizable, ::mlir::InferTypeOpInterface::Trait> {
public:
 using Op::Op;
 static FMAOp create(OpBuilder &,Location,Type,Value,Value,Value);
};
} }


namespace mlir { namespace vector {
 



} }

namespace mlir { namespace vector {
 



} }
