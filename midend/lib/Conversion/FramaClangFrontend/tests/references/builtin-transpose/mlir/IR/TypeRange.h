#ifndef MLIR_IR_TYPERANGE_H
#define MLIR_IR_TYPERANGE_H

#include "mlir/IR/Types.h"

namespace mlir {

class TypeRange {
public:
  TypeRange() = default;

  template <typename... Args>
  TypeRange(Args &&...) {}
};

} // namespace mlir

#endif
