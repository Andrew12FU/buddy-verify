#ifndef FC_MLIR_INTERFACES_CALLINTERFACES_H
#define FC_MLIR_INTERFACES_CALLINTERFACES_H

#include "mlir/IR/Operation.h"

namespace mlir {

/* 官方 CallOpInterface : OpInterface<CallOpInterface, ...>。
 * 模型只保留 isa<CallOpInterface>(op) 需要的 classof（不展开 OpInterface 机制）。 */
class CallOpInterface {
public:
  CallOpInterface();

  static bool classof(Operation *op) { return false; }

private:
  int reserved;
};

} // namespace mlir
#endif
