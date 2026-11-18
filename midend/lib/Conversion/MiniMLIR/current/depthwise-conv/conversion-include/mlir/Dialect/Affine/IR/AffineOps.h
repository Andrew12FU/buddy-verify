// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations; algorithms require external specifications.
#pragma once
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/IntegerSet.h"
#include "mlir/IR/TypeRange.h"
namespace mlir { namespace affine { class AffineDialect : public Dialect { public: explicit AffineDialect(MLIRContext *context); }; } }
namespace mlir { namespace affine {
class AffineApplyOp : public ::mlir::Op<AffineApplyOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::IndexType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::InferIntDivisibilityOpInterface::Trait, ::mlir::InferIntRangeInterface::Trait, ::mlir::InferTypeOpInterface::Trait> {
public:
 using Op::Op;
 static AffineApplyOp create(OpBuilder &,Location,AffineMap,ValueRange);
};
} }


namespace mlir { namespace affine {
 



} }

namespace mlir { namespace affine {
 




} }

namespace mlir { namespace affine {
 



} }

namespace mlir { namespace affine {
 
 
} }
