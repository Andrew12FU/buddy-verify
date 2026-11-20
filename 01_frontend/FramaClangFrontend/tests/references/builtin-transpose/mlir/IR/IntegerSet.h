#ifndef MLIR_IR_INTEGERSET_H
#define MLIR_IR_INTEGERSET_H

#include "mlir/IR/AffineExpr.h"
#include "mlir/Support/LLVM.h"

namespace mlir {

class IntegerSet {
public:
  static IntegerSet get(unsigned, unsigned, ArrayRef<AffineExpr>,
                        ArrayRef<bool>) {
    return IntegerSet();
  }
};

} // namespace mlir

#endif
