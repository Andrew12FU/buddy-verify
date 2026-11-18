// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_BUILTINOPS_H
#define BUDDY_MINI_MLIR_IR_BUILTINOPS_H

#include "mlir/IR/OpDefinition.h"

// Provenance: audited Reshape V2 mini, line 1626; see docs/PROVENANCE.md.
namespace mlir {
class ModuleOp;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1627; see docs/PROVENANCE.md.
namespace mlir {
template class ::mlir::Op<
    ModuleOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::ZeroResults,
    ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::ZeroOperands,
    ::mlir::OpTrait::NoRegionArguments, ::mlir::OpTrait::NoTerminator,
    ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::OpInvariants,
    ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AffineScope,
    ::mlir::OpTrait::IsIsolatedFromAbove, ::mlir::OpTrait::SymbolTable,
    ::mlir::SymbolOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait,
    ::mlir::RegionKindInterface::Trait, ::mlir::OpTrait::HasOnlyGraphRegion>;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1628; see docs/PROVENANCE.md.
namespace mlir {
class ModuleOp
    : public ::mlir::Op<
          ModuleOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::ZeroResults,
          ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::ZeroOperands,
          ::mlir::OpTrait::NoRegionArguments, ::mlir::OpTrait::NoTerminator,
          ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::OpInvariants,
          ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AffineScope,
          ::mlir::OpTrait::IsIsolatedFromAbove, ::mlir::OpTrait::SymbolTable,
          ::mlir::SymbolOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait,
          ::mlir::RegionKindInterface::Trait,
          ::mlir::OpTrait::HasOnlyGraphRegion> {
public:
  using Op::Op;
};
} // namespace mlir

#endif
