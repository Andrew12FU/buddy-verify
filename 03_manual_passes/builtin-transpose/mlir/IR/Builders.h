#ifndef MLIR_IR_BUILDERS_H
#define MLIR_IR_BUILDERS_H

#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/LLVM.h"

#include <cstdint>

namespace mlir {

class OpBuilder {
public:
  MLIRContext *getContext() const { return nullptr; }

  NamedAttribute getNamedAttr(StringRef, Attribute) const {
    return NamedAttribute();
  }
  Attribute getI32TensorAttr(ArrayRef<int>) const { return Attribute(); }
  Attribute getBoolArrayAttr(ArrayRef<bool>) const { return Attribute(); }
  Attribute getDenseI32ArrayAttr(ArrayRef<int>) const { return Attribute(); }
  Attribute getI64ArrayAttr(ArrayRef<int64_t>) const { return Attribute(); }
  Attribute getArrayAttr(ArrayRef<Attribute>) const { return Attribute(); }
  Attribute getIndexAttr(int64_t) const { return Attribute(); }
  Attribute getZeroAttr(Type) const { return Attribute(); }
  StringAttr getStringAttr(const char *) const { return StringAttr(); }
  AffineExpr getAffineDimExpr(unsigned) const { return AffineExpr(); }
  AffineExpr getAffineSymbolExpr(unsigned) const { return AffineExpr(); }
  AffineExpr getAffineConstantExpr(int64_t) const { return AffineExpr(); }
  Type getI1Type() const { return Type(); }
  Type getIndexType() const { return Type(); }
};

} // namespace mlir

#endif
