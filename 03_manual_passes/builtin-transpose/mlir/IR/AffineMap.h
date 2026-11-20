#ifndef MLIR_IR_AFFINEMAP_H
#define MLIR_IR_AFFINEMAP_H

#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/LLVM.h"

namespace mlir {

class AffineMap {
public:
  static AffineMap get(unsigned, unsigned, AffineExpr) { return AffineMap(); }

  static AffineMap get(unsigned, unsigned, ArrayRef<AffineExpr>) {
    return AffineMap();
  }

  static AffineMap get(unsigned, unsigned, ArrayRef<AffineExpr>, MLIRContext *) {
    return AffineMap();
  }
};

} // namespace mlir

#endif
