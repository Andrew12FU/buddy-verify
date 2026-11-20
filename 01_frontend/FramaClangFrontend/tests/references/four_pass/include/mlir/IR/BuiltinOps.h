#ifndef FC_MLIR_IR_BUILTINOPS_H
#define FC_MLIR_IR_BUILTINOPS_H

#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"

namespace mlir {

/* 官方 ModuleOp : Op<ModuleOp>。 */
class ModuleOp : public Op<ModuleOp> {
public:
  ModuleOp() : Op<ModuleOp>(0) {}
   explicit ModuleOp(Operation *op) : Op<ModuleOp>(op) {}

  static StringRef getOperationName();
  static ModuleOp create(Location loc);

  Region &getBodyRegion();
  Block *getBody(unsigned idx = 0);
};

} // namespace mlir
#endif
