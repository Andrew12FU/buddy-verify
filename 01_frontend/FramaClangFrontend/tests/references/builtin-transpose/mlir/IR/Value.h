#ifndef MLIR_IR_VALUE_H
#define MLIR_IR_VALUE_H

#include "mlir/IR/Types.h"

namespace mlir {

class Value {
public:
  Type getType() const { return Type(); }
};

} // namespace mlir

#endif
