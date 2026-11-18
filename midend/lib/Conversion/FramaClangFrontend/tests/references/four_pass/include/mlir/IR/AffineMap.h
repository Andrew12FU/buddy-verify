#ifndef FC_MLIR_IR_AFFINEMAP_H
#define FC_MLIR_IR_AFFINEMAP_H

#include "mlir/IR/AffineExpr.h"

namespace mlir {

class AffineMap {
public:
  int reserved;

  AffineMap();

  static AffineMap get(int64_t dimCount, int64_t symbolCount, AffineExpr expr);
};

} // namespace mlir
#endif
