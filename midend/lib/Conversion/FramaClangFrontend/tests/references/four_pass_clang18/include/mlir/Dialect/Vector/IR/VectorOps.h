#ifndef FC_MLIR_DIALECT_VECTOR_IR_VECTOROPS_H
#define FC_MLIR_DIALECT_VECTOR_IR_VECTOROPS_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {

class VectorDialect : public Dialect {
public:
  VectorDialect();
};

namespace vector {

class LoadOp : public Op<LoadOp> {
public:
  LoadOp() : Op<LoadOp>(0) {}
   explicit LoadOp(Operation *op) : Op<LoadOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static LoadOp create(OpBuilder &builder, Location loc, VectorType vectorType,
                       Value base, ValueRange indices);
};

class StoreOp : public Op<StoreOp> {
public:
  StoreOp() : Op<StoreOp>(0) {}
   explicit StoreOp(Operation *op) : Op<StoreOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static StoreOp create(OpBuilder &builder, Location loc, Value value,
                        Value base, ValueRange indices);
};

class BroadcastOp : public Op<BroadcastOp> {
public:
  BroadcastOp() : Op<BroadcastOp>(0) {}
   explicit BroadcastOp(Operation *op) : Op<BroadcastOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static BroadcastOp create(OpBuilder &builder, Location loc,
                            VectorType vectorType, Value source);
};

class CreateMaskOp : public Op<CreateMaskOp> {
public:
  CreateMaskOp() : Op<CreateMaskOp>(0) {}
   explicit CreateMaskOp(Operation *op) : Op<CreateMaskOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  /* 官方第二参是 ArrayRef<OpFoldResult>；模型收 ValueRange（调用点传单值），
   * 见 FORCED_DEVIATIONS。 */
  static CreateMaskOp create(OpBuilder &builder, Location loc,
                             VectorType vectorType, ValueRange lengths);
};

class MaskedLoadOp : public Op<MaskedLoadOp> {
public:
  MaskedLoadOp() : Op<MaskedLoadOp>(0) {}
   explicit MaskedLoadOp(Operation *op) : Op<MaskedLoadOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static MaskedLoadOp create(OpBuilder &builder, Location loc,
                             VectorType vectorType, Value base,
                             ValueRange indices, Value mask, Value passThru);
};

class MaskedStoreOp : public Op<MaskedStoreOp> {
public:
  MaskedStoreOp() : Op<MaskedStoreOp>(0) {}
   explicit MaskedStoreOp(Operation *op) : Op<MaskedStoreOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static MaskedStoreOp create(OpBuilder &builder, Location loc, Value base,
                              ValueRange indices, Value mask, Value value);
};

class FMAOp : public Op<FMAOp> {
public:
  FMAOp() : Op<FMAOp>(0) {}
   explicit FMAOp(Operation *op) : Op<FMAOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static FMAOp create(OpBuilder &builder, Location loc, VectorType vectorType,
                      Value lhs, Value rhs, Value acc);
};

} // namespace vector
} // namespace mlir
#endif
