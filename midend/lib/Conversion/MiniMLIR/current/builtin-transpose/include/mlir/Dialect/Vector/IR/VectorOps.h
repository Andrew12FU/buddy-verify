// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations; algorithms require external specifications.
#pragma once
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
namespace mlir { namespace vector { class VectorDialect : public Dialect { public: explicit VectorDialect(MLIRContext *context); }; } }
namespace mlir { namespace vector {
 




} }
namespace mlir { namespace vector {
 




} }
namespace mlir { namespace vector {
 




} }
namespace mlir { namespace vector {
class CreateMaskOp : public ::mlir::Op<CreateMaskOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait> {
public:
 using Op::Op;
 static CreateMaskOp create(OpBuilder &,Location,Type,ValueRange); static CreateMaskOp create(OpBuilder &,Location,VectorType,ArrayRef<OpFoldResult>);
};
} }
namespace mlir { namespace vector {
 




} }
namespace mlir { namespace vector {
 




} }
namespace mlir { namespace vector {
 




} }


namespace mlir { namespace vector {
class TransferReadOp : public ::mlir::Op<TransferReadOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::VectorType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<2>::Impl, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::VectorTransferOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::vector::MaskableOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::DestinationStyleOpInterface::Trait> {
public: using Op::Op;
 static TransferReadOp create(OpBuilder &,Location,TypeRange,ValueRange,ArrayRef<NamedAttribute> attributes={});
};
} }

namespace mlir { namespace vector {
class TransferWriteOp : public ::mlir::Op<TransferWriteOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<2>::Impl, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::VectorTransferOpInterface::Trait, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::vector::MaskableOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::MemorySpaceCastConsumerOpInterface::Trait, ::mlir::DestinationStyleOpInterface::Trait> {
public: using Op::Op;
 static TransferWriteOp create(OpBuilder &,Location,TypeRange,ValueRange,ArrayRef<NamedAttribute> attributes={});
};
} }
