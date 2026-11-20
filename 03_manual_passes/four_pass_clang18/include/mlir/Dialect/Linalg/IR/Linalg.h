#ifndef FC_MLIR_DIALECT_LINALG_IR_LINALG_H
#define FC_MLIR_DIALECT_LINALG_IR_LINALG_H

#include "mlir/Dialect/Linalg/IR/LinalgOps.h"
#include "mlir/IR/Dialect.h"

namespace mlir {
namespace linalg {

class LinalgDialect : public Dialect {
public:
  LinalgDialect();
};

} // namespace linalg
} // namespace mlir
#endif
