// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_BUILDERS_H
#define BUDDY_MINI_MLIR_IR_BUILDERS_H

#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Operation.h"

// Provenance: audited Reshape V2 mini, line 1464; see docs/PROVENANCE.md.
namespace mlir {
class Builder {
public:
  inline explicit Builder(MLIRContext *context);
  inline explicit Builder(Operation *op);
  inline MLIRContext *getContext() const;
  TypedAttr getZeroAttr(Type type);
  IntegerType getI1Type();
  IndexType getIndexType();
  AffineExpr getAffineSymbolExpr(unsigned position);
  AffineExpr getAffineConstantExpr(int64_t value);
  StringAttr getStringAttr(const llvm::Twine &bytes);
  NamedAttribute getNamedAttr(StringRef name, Attribute value);
  ArrayAttr getArrayAttr(ArrayRef<Attribute> value);
  DenseIntElementsAttr getI32TensorAttr(ArrayRef<int32_t> values);
  DenseI32ArrayAttr getDenseI32ArrayAttr(ArrayRef<int32_t> values);
  ArrayAttr getBoolArrayAttr(ArrayRef<bool> values);
  ArrayAttr getI64ArrayAttr(ArrayRef<int64_t> values);
  FloatAttr getFloatAttr(Type type, double value);
  AffineExpr getAffineDimExpr(unsigned position);
  Location getUnknownLoc();
  IntegerAttr getIndexAttr(int64_t value);
  FunctionType getFunctionType(TypeRange inputs,TypeRange results);

protected:
  MLIRContext *context;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1472; see docs/PROVENANCE.md.
namespace mlir {
class OpBuilder : public Builder {
public:
  struct Listener;
  void setInsertionPoint(Operation *op);
  void setInsertionPointAfter(Operation *op);
  void setInsertionPointToStart(Block *block);
  inline explicit OpBuilder(MLIRContext *ctx, Listener *listener = nullptr);
  inline explicit OpBuilder(Region *region, Listener *listener = nullptr);
  inline explicit OpBuilder(Region &region, Listener *listener = nullptr);
  inline explicit OpBuilder(Operation *op, Listener *listener = nullptr);

protected:
  Listener *listener;

private:
  // Block::iterator is outside current calls; omitted, not replaced by a
  // pointer.
};
} // namespace mlir

#endif
