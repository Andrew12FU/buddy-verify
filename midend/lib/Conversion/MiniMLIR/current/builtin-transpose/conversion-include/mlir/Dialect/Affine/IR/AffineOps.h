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
class AffineYieldOp : public ::mlir::Op<AffineYieldOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::OpTrait::IsTerminator, ::mlir::RegionBranchTerminatorOpInterface::Trait, ::mlir::OpTrait::ReturnLike, ::mlir::OpTrait::MemRefsNormalizable> {
public: using Op::Op;
 static AffineYieldOp create(OpBuilder &,Location);
};
} }

namespace mlir { namespace affine {
class AffineIfOp : public ::mlir::Op<AffineIfOp, ::mlir::OpTrait::NRegions<2>::Impl, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<AffineYieldOp>::Impl, ::mlir::OpTrait::NoRegionArguments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::RecursivelySpeculatableImplTrait, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::RegionBranchOpInterface::Trait> {
public: using Op::Op;
 static AffineIfOp create(OpBuilder &,Location,IntegerSet,ValueRange,bool);
 OpBuilder getThenBodyBuilder(OpBuilder::Listener *listener=nullptr);
};
} }

namespace mlir { namespace affine {
class AffineParallelOp : public ::mlir::Op<AffineParallelOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<AffineYieldOp>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AutomaticAllocationScope, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::RecursivelySpeculatableImplTrait, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::LoopLikeOpInterface::Trait, ::mlir::OpTrait::MemRefsNormalizable> {
public: using Op::Op;
 static AffineParallelOp create(OpBuilder &,Location,TypeRange,ValueRange,ArrayRef<NamedAttribute> attributes={});
};
} }

namespace mlir { namespace affine {
void buildAffineLoopNest(OpBuilder &builder,Location loc,ArrayRef<int64_t> lbs,ArrayRef<int64_t> ubs,ArrayRef<int64_t> steps,function_ref<void(OpBuilder &,Location,ValueRange)> bodyBuilderFn=nullptr);
void buildAffineLoopNest(OpBuilder &builder,Location loc,ValueRange lbs,ValueRange ubs,ArrayRef<int64_t> steps,function_ref<void(OpBuilder &,Location,ValueRange)> bodyBuilderFn=nullptr);
} }
