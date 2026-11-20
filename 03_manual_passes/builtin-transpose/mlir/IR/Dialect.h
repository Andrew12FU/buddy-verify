#ifndef MLIR_IR_DIALECT_H
#define MLIR_IR_DIALECT_H

namespace mlir {

class DialectRegistry {
public:
  template <typename... Dialects>
  void insert() {}
};

} // namespace mlir

#endif
