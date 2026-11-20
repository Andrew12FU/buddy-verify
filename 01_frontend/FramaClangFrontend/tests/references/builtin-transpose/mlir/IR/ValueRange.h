#ifndef MLIR_IR_VALUERANGE_H
#define MLIR_IR_VALUERANGE_H

#include "llvm/ADT/SmallVector.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Value.h"

namespace mlir {

class ValueRange {
public:
  ValueRange() = default;

  template <typename... Args>
  ValueRange(Args &&...) {}

  TypeRange getTypes() const { return TypeRange(); }
  Value front() const { return Value(); }
};

} // namespace mlir

#endif
