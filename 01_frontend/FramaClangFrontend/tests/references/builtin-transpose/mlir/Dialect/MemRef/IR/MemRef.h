#ifndef MLIR_DIALECT_MEMREF_IR_MEMREF_H
#define MLIR_DIALECT_MEMREF_IR_MEMREF_H

#include "mlir/IR/Value.h"

namespace mlir {
namespace memref {

class MemRefDialect {};

class DimOp {
public:
  template <typename... Args>
  static Value create(Args &&...) {
    return Value();
  }
};

} // namespace memref
} // namespace mlir

#endif
