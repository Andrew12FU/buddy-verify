// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_DIALECT_FUNC_IR_FUNCOPS_H
#define BUDDY_MINI_MLIR_DIALECT_FUNC_IR_FUNCOPS_H

#include "mlir/IR/OpDefinition.h"

// Provenance: audited Reshape V2 mini, line 1773; see docs/PROVENANCE.md.
namespace mlir {
namespace func {
class FuncOp
    : public Op<FuncOp, OpTrait::OneRegion, OpTrait::ZeroResults,
                OpTrait::ZeroSuccessors, OpTrait::ZeroOperands,
                OpTrait::OpInvariants, BytecodeOpInterface::Trait,
                OpTrait::AffineScope, OpTrait::AutomaticAllocationScope,
                SymbolOpInterface::Trait, ArgAndResultAttrsOpInterface::Trait,
                CallableOpInterface::Trait, FunctionOpInterface::Trait,
                OpTrait::IsIsolatedFromAbove, OpAsmOpInterface::Trait> {
public:
  using Op::Op;
  Region &getBody();
  FunctionType getFunctionType();
  ArrayRef<Type> getArgumentTypes();
  bool isDeclaration();
  Region::iterator begin();
  Region::iterator end();
};
} // namespace func
} // namespace mlir

namespace mlir { namespace func {
class ReturnOp : public ::mlir::Op<ReturnOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::HasParent<FuncOp>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::OpTrait::MemRefsNormalizable, ::mlir::RegionBranchTerminatorOpInterface::Trait, ::mlir::OpTrait::ReturnLike, ::mlir::OpTrait::IsTerminator> { public: using Op::Op; };
} }

#endif
