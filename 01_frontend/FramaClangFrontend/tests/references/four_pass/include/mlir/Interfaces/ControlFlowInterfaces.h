#ifndef FC_MLIR_INTERFACES_CONTROLFLOWINTERFACES_H
#define FC_MLIR_INTERFACES_CONTROLFLOWINTERFACES_H

#include "mlir/IR/Operation.h"

namespace mlir {

/* 官方 BranchOpInterface / RegionBranchOpInterface 都是 OpInterface；
 * 模型只保留 isa<...>(op) 需要的 classof。 */
class BranchOpInterface {
public:
  BranchOpInterface();

  static bool classof(Operation *op) { return false; }

private:
  int reserved;
};

class RegionBranchOpInterface {
public:
  RegionBranchOpInterface();

  static bool classof(Operation *op) { return false; }

private:
  int reserved;
};

} // namespace mlir
#endif
