#ifndef MLIR_DIALECT_AFFINE_ANALYSIS_AFFINEANALYSIS_H
#define MLIR_DIALECT_AFFINE_ANALYSIS_AFFINEANALYSIS_H

#include "mlir/IR/Value.h"

namespace mlir {

struct LoopReduction {
  Value value;
};

} // namespace mlir

#endif
