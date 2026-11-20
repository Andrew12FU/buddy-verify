#ifndef FC_MLIR_IR_BLOCK_H
#define FC_MLIR_IR_BLOCK_H

#include "llvm/ADT/ArrayRef.h"

#include "mlir/IR/Value.h"

namespace mlir {

class Operation;

/* 官方 Block：BlockArgListType 是类内成员 typedef，且是可变视图。 */
class Block {
public:
  typedef llvm::MutableArrayRef<BlockArgument> BlockArgListType;

  Block();

  Operation *getParentOp() const;
  BlockArgListType getArguments();

private:
  void *impl;
};

} // namespace mlir
#endif
