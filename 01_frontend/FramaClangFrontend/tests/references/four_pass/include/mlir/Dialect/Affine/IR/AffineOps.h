#ifndef FC_MLIR_DIALECT_AFFINE_IR_AFFINEOPS_H
#define FC_MLIR_DIALECT_AFFINE_IR_AFFINEOPS_H

#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {
namespace affine {

class AffineDialect : public Dialect {
public:
  AffineDialect();
};

class AffineApplyOp : public Op<AffineApplyOp> {
public:
  AffineApplyOp() : Op<AffineApplyOp>(0) {}
   explicit AffineApplyOp(Operation *op) : Op<AffineApplyOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static AffineApplyOp create(OpBuilder &builder, Location loc, AffineMap map,
                              ValueRange operands);
};

} // namespace affine
} // namespace mlir
#endif
