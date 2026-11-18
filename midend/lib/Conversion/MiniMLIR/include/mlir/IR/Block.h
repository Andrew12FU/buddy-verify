// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations from the local LLVM/MLIR checkout.
#ifndef BUDDY_MINI_MLIR_IR_BLOCK_H
#define BUDDY_MINI_MLIR_IR_BLOCK_H
#include "mlir/IR/ValueRange.h"
#include "llvm/ADT/ilist_node.h"
namespace mlir {
class Block : public llvm::ilist_node_with_parent<Block, Region> {
public:
  Block();
  ~Block();
  BlockArgument addArgument(Type type, Location loc);
  using BlockArgListType = llvm::MutableArrayRef<BlockArgument>;
  BlockArgListType getArguments();
  Operation *getParentOp();
};
}

#endif
