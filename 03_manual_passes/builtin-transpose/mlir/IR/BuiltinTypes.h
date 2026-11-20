#ifndef MLIR_IR_BUILTINTYPES_H
#define MLIR_IR_BUILTINTYPES_H

#include "mlir/IR/Types.h"
#include "mlir/Support/LLVM.h"

#include <cstdint>

namespace mlir {

class VectorType : public Type {
public:
  static VectorType get(ArrayRef<int64_t>, Type) { return VectorType(); }
};

class MemRefType : public Type {
public:
  int64_t getRank() const { return 0; }
  Type getElementType() const { return Type(); }
  bool isDynamicDim(unsigned) const { return false; }
  int64_t getDimSize(unsigned) const { return 0; }
};

} // namespace mlir

#endif
