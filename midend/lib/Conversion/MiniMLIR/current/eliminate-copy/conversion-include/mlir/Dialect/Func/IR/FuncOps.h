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
   
   
   
   
  Region::iterator begin();
   
};
} // namespace func
} // namespace mlir

namespace mlir { namespace func {
 
} }

#endif
