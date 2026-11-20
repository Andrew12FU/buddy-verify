// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_REGION_H
#define BUDDY_MINI_MLIR_IR_REGION_H

#include "llvm/ADT/ilist.h"
#include "mlir/IR/Block.h"

// Provenance: audited Reshape V2 mini, line 1203; see docs/PROVENANCE.md.
namespace mlir {
class Region {
public:
  Region() = default;
  explicit Region(Operation *container);
  ~Region();
  using BlockListType = llvm::iplist<Block>;
  using iterator = BlockListType::iterator;
  void push_back(Block *block);
   
  iterator begin();
  iterator end();

private:
  BlockListType blocks;
  Operation *container = nullptr;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1463; see docs/PROVENANCE.md.
namespace mlir {
class Region;
} // namespace mlir

#endif
