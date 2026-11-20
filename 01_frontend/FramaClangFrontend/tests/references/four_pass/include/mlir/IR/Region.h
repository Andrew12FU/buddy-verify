#ifndef FC_MLIR_IR_REGION_H
#define FC_MLIR_IR_REGION_H

#include "mlir/IR/Block.h"

namespace mlir {

class Region {
public:
  typedef Block *iterator;

  Region();

  iterator begin();
  iterator end();
  Block &front();
  Block &back();
  bool empty();
  unsigned getNumArguments();

private:
  void *impl;
};

} // namespace mlir
#endif
