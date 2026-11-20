#ifndef MLIR_IR_BUILTINATTRIBUTES_H
#define MLIR_IR_BUILTINATTRIBUTES_H

#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Attributes.h"
#include "mlir/Support/LLVM.h"

#include <cstdint>

namespace mlir {

class StringAttr : public Attribute {};

class DenseI64ArrayAttr : public Attribute {
public:
  ArrayRef<int64_t> asArrayRef() const { return ArrayRef<int64_t>(); }
};

class AffineMapAttr {
public:
  static Attribute get(AffineMap) { return Attribute(); }
};

} // namespace mlir

#endif
