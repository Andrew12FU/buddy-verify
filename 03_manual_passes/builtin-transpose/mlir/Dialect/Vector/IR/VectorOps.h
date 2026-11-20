#ifndef MLIR_DIALECT_VECTOR_IR_VECTOROPS_H
#define MLIR_DIALECT_VECTOR_IR_VECTOROPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {

class VectorDialect {};

namespace vector {

class CreateMaskOp {
public:
  template <typename... Args>
  static Value create(Args &&...) {
    return Value();
  }
};

class TransferReadOp {
public:
  template <typename... Args>
  static Value create(Args &&...) {
    return Value();
  }
};

class TransferWriteOp {
public:
  template <typename... Args>
  static void create(Args &&...) {}
};

} // namespace vector
} // namespace mlir

#endif
