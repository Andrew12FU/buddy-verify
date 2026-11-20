#ifndef MLIR_DIALECT_LINALG_TRANSFORMS_TRANSFORMS_H
#define MLIR_DIALECT_LINALG_TRANSFORMS_TRANSFORMS_H

#include "mlir/Support/LLVM.h"

namespace mlir {
namespace linalg {

class LinalgDialect {};
class FillOp {};

class TransposeOp {
public:
  static StringRef getOperationName() { return StringRef("linalg.transpose"); }
};

} // namespace linalg
} // namespace mlir

#endif
