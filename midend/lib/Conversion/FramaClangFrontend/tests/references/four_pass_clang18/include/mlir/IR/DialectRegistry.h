#ifndef FC_MLIR_IR_DIALECTREGISTRY_H
#define FC_MLIR_IR_DIALECTREGISTRY_H

#include "mlir/IR/MLIRContext.h"

namespace mlir {

class DialectRegistry {
public:
  int reserved;

  DialectRegistry();

  template <class... Dialects>
  void insert() {}
};

} // namespace mlir
#endif
