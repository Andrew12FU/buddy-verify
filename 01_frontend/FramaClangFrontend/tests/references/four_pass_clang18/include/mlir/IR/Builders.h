#ifndef FC_MLIR_IR_BUILDERS_H
#define FC_MLIR_IR_BUILDERS_H

#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/TypeRange.h"

namespace mlir {

/* 官方 Builder：context 级的属性/类型构造函数。 */
class Builder {
public:
  explicit Builder(MLIRContext *context) : context(context) {}

  MLIRContext *getContext() const { return context; }

  Location getUnknownLoc();

  IntegerAttr getIndexAttr(int64_t value);
  TypedAttr getZeroAttr(Type type);
  IntegerType getI1Type();

  AffineExpr getAffineDimExpr(unsigned position);
  AffineExpr getAffineSymbolExpr(unsigned position);

  FunctionType getFunctionType(TypeRange inputs, TypeRange results);

protected:
  MLIRContext *context;
};

/* 官方 OpBuilder：插入点管理 + create。 */
class OpBuilder : public Builder {
public:
  class Listener {
  public:
    Listener();
    virtual ~Listener();
  };

  explicit OpBuilder(MLIRContext *ctx, Listener *listener = 0)
      : Builder(ctx), listener(listener) {}

  void setInsertionPoint(Operation *op);
  void setInsertionPointAfter(Operation *op);
  void setInsertionPointToStart(Block *block);
  void setInsertionPointToEnd(Block *block);

protected:
  Listener *listener;
};

} // namespace mlir
#endif
