#ifndef FC_MLIR_DIALECT_MEMREF_IR_MEMREF_H
#define FC_MLIR_DIALECT_MEMREF_IR_MEMREF_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir {
namespace memref {

class MemRefDialect : public Dialect {
public:
  MemRefDialect();
};

class DimOp : public Op<DimOp> {
public:
  DimOp() : Op<DimOp>(0) {}
   explicit DimOp(Operation *op) : Op<DimOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static DimOp create(OpBuilder &builder, Location loc, Value source,
                      int64_t index);
};

class CopyOp : public Op<CopyOp> {
public:
  CopyOp() : Op<CopyOp>(0) {}
   explicit CopyOp(Operation *op) : Op<CopyOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static CopyOp create(OpBuilder &builder, Location loc, Value source,
                       Value target);

  Value getSource();
  Value getTarget();
};

class AllocOp : public Op<AllocOp> {
public:
  AllocOp() : Op<AllocOp>(0) {}
   explicit AllocOp(Operation *op) : Op<AllocOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static AllocOp create(OpBuilder &builder, Location loc,
                        MemRefType memrefType,
                        ValueRange dynamicSizes = ValueRange());
};

class DeallocOp : public Op<DeallocOp> {
public:
  DeallocOp() : Op<DeallocOp>(0) {}
   explicit DeallocOp(Operation *op) : Op<DeallocOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  Value getMemref();
};

class GetGlobalOp : public Op<GetGlobalOp> {
public:
  GetGlobalOp() : Op<GetGlobalOp>(0) {}
   explicit GetGlobalOp(Operation *op) : Op<GetGlobalOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);
};

class CastOp : public Op<CastOp> {
public:
  CastOp() : Op<CastOp>(0) {}
   explicit CastOp(Operation *op) : Op<CastOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static CastOp create(OpBuilder &builder, Location loc, Type dest,
                       Value source);

  Value getSource();
};

class ReinterpretCastOp : public Op<ReinterpretCastOp> {
public:
  ReinterpretCastOp() : Op<ReinterpretCastOp>(0) {}
   explicit ReinterpretCastOp(Operation *op) : Op<ReinterpretCastOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static ReinterpretCastOp create(OpBuilder &builder, Location loc,
                                 MemRefType resultType, Value source,
                                 OpFoldResult offset,
                                 llvm::ArrayRef<OpFoldResult> sizes,
                                 llvm::ArrayRef<OpFoldResult> strides);

  Value getSource();
};

class ExtractStridedMetadataOp : public Op<ExtractStridedMetadataOp> {
public:
  ExtractStridedMetadataOp() : Op<ExtractStridedMetadataOp>(0) {}
  explicit ExtractStridedMetadataOp(Operation *op)
      : Op<ExtractStridedMetadataOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static ExtractStridedMetadataOp create(OpBuilder &builder, Location loc,
                                        Value source);

  /* 官方：getSizes()/getStrides() 返回 result_range（Value 视图），
   * getOffset() 返回 Value；OpFoldResult 版本叫 getConstifiedMixed*。 */
  OperandRange getSizes();
  OperandRange getStrides();
  Value getOffset();
  llvm::SmallVector<OpFoldResult> getConstifiedMixedSizes();
  llvm::SmallVector<OpFoldResult> getConstifiedMixedStrides();
  OpFoldResult getConstifiedMixedOffset();
  Value getBaseBuffer();
  Value getSource();
};

} // namespace memref
} // namespace mlir
#endif
