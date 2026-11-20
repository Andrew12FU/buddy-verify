#ifndef MLIR_DIALECT_ARITH_IR_ARITH_H
#define MLIR_DIALECT_ARITH_IR_ARITH_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Value.h"

namespace mlir {
namespace arith {

class ArithDialect {};

class ConstantOp {
public:
  template <typename... Args>
  static Value create(Args &&...) {
    return Value();
  }
};

} // namespace arith
} // namespace mlir

#endif
