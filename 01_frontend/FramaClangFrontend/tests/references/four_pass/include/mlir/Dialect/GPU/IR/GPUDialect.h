#ifndef FC_MLIR_DIALECT_GPU_IR_GPUDIALECT_H
#define FC_MLIR_DIALECT_GPU_IR_GPUDIALECT_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {
namespace gpu {

class GPUDialect : public Dialect {
public:
  GPUDialect();
};

enum class AddressSpace : unsigned { Global = 1, Workgroup = 3, Private = 5 };

class AddressSpaceAttr : public Attribute {
public:
  AddressSpaceAttr();
  AddressSpaceAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);

  AddressSpace getValue() const;
};

class LaunchOp : public Op<LaunchOp> {
public:
  LaunchOp() : Op<LaunchOp>(0) {}
   explicit LaunchOp(Operation *op) : Op<LaunchOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);
};

class AllocOp : public Op<AllocOp> {
public:
  AllocOp() : Op<AllocOp>(0) {}
   explicit AllocOp(Operation *op) : Op<AllocOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  /* 官方 collective 形态：(TypeRange resultTypes, ValueRange operands, attrs = {})。 */
  static AllocOp create(OpBuilder &builder, Location loc, TypeRange resultTypes,
                        ValueRange operands);
};

class MemcpyOp : public Op<MemcpyOp> {
public:
  MemcpyOp() : Op<MemcpyOp>(0) {}
   explicit MemcpyOp(Operation *op) : Op<MemcpyOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  /* 官方逐参形态：(Type asyncToken, ValueRange asyncDependencies, Value dst, Value src)。 */
  static MemcpyOp create(OpBuilder &builder, Location loc, Type asyncToken,
                         ValueRange asyncDependencies, Value dst, Value src);
};

class DeallocOp : public Op<DeallocOp> {
public:
  DeallocOp() : Op<DeallocOp>(0) {}
   explicit DeallocOp(Operation *op) : Op<DeallocOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  /* 官方逐参形态：(Type asyncToken, ValueRange asyncDependencies, Value memref)。 */
  static DeallocOp create(OpBuilder &builder, Location loc, Type asyncToken,
                          ValueRange asyncDependencies, Value memref);
};

} // namespace gpu
} // namespace mlir
#endif
