// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations; algorithms require external specifications.
#pragma once
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
namespace mlir { namespace scf { class SCFDialect : public Dialect { public: explicit SCFDialect(MLIRContext *context); }; } }
namespace mlir { namespace scf { class YieldOp; class ForallOp;class ExecuteRegionOp;class IndexSwitchOp;class WhileOp;class ForOp;class IfOp; } }
namespace mlir { namespace scf {
class YieldOp : public ::mlir::Op<YieldOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::HasParent<ExecuteRegionOp, ForOp, IfOp, IndexSwitchOp, WhileOp>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::RegionBranchTerminatorOpInterface::Trait, ::mlir::OpTrait::ReturnLike, ::mlir::OpTrait::IsTerminator> {
public:
 using Op::Op;
 static YieldOp create(OpBuilder &,Location,ValueRange);
};
} }
namespace mlir { namespace scf {
class InParallelOp : public ::mlir::Op<InParallelOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::ZeroOperands, ::mlir::OpTrait::HasParent<ForallOp>::Impl, ::mlir::OpTrait::NoTerminator, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::OpTrait::IsTerminator, ::mlir::InParallelOpInterface::Trait, ::mlir::RegionBranchTerminatorOpInterface::Trait, ::mlir::OpTrait::ReturnLike, ::mlir::RegionKindInterface::Trait, ::mlir::OpTrait::HasOnlyGraphRegion> {
public:
 using Op::Op;
 static InParallelOp create(OpBuilder &,Location);
};
} }
namespace mlir { namespace scf {
class ForOp : public ::mlir::Op<ForOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<3>::Impl, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<scf::YieldOp>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AutomaticAllocationScope, ::mlir::LoopLikeOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::RegionBranchOpInterface::Trait, ::mlir::PromotableRegionOpInterface::Trait, ::mlir::OpTrait::HasRecursiveMemoryEffects> {
public:
 using Op::Op;
 static ForOp create(OpBuilder &,Location,Value,Value,Value,ValueRange initArgs = {},function_ref<void(OpBuilder &,Location,Value,ValueRange)> bodyBuilder = nullptr,bool unsignedCmp = false);
};
} }
namespace mlir { namespace scf {
class ForallOp : public ::mlir::Op<ForallOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<scf::InParallelOp>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AutomaticAllocationScope, ::mlir::LoopLikeOpInterface::Trait, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::RegionBranchOpInterface::Trait, ::mlir::PromotableRegionOpInterface::Trait, ::mlir::DestinationStyleOpInterface::Trait, ::mlir::OpTrait::HasParallelRegion> {
public:
 using Op::Op;
 static ForallOp create(OpBuilder &,Location,ArrayRef<OpFoldResult>,ValueRange,std::optional<ArrayAttr>,function_ref<void(OpBuilder &,Location,ValueRange)> bodyBuilder = nullptr);
};
} }
namespace mlir { namespace scf {
class IfOp : public ::mlir::Op<IfOp, ::mlir::OpTrait::NRegions<2>::Impl, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<scf::YieldOp>::Impl, ::mlir::OpTrait::NoRegionArguments, ::mlir::OpTrait::OpInvariants, ::mlir::RegionBranchOpInterface::Trait, ::mlir::PromotableRegionOpInterface::Trait, ::mlir::InferTypeOpInterface::Trait, ::mlir::OpTrait::InferTypeOpAdaptor, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::RecursivelySpeculatableImplTrait> {
public:
 using Op::Op;
 static IfOp create(OpBuilder &,Location,Value,function_ref<void(OpBuilder &,Location)> thenBuilder = buildTerminatedBody,function_ref<void(OpBuilder &,Location)> elseBuilder = nullptr); static void buildTerminatedBody(OpBuilder &,Location);
};
} }

