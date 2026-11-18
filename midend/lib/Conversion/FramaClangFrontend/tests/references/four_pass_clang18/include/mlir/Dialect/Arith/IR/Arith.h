#ifndef FC_MLIR_DIALECT_ARITH_IR_ARITH_H
#define FC_MLIR_DIALECT_ARITH_IR_ARITH_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir {
namespace arith {

class ArithDialect : public Dialect {
public:
  ArithDialect();
};

enum class CmpIPredicate { eq, ne, slt, sle, sgt, sge, ult, ule, ugt, uge };

class ConstantOp : public Op<ConstantOp> {
public:
  ConstantOp() : Op<ConstantOp>(0) {}
   explicit ConstantOp(Operation *op) : Op<ConstantOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static ConstantOp create(OpBuilder &builder, Location loc, Attribute value);
};

class MulIOp : public Op<MulIOp> {
public:
  MulIOp() : Op<MulIOp>(0) {}
   explicit MulIOp(Operation *op) : Op<MulIOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static MulIOp create(OpBuilder &builder, Location loc, Value lhs, Value rhs);
};

class AddIOp : public Op<AddIOp> {
public:
  AddIOp() : Op<AddIOp>(0) {}
   explicit AddIOp(Operation *op) : Op<AddIOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static AddIOp create(OpBuilder &builder, Location loc, Value lhs, Value rhs);
};

class CmpIOp : public Op<CmpIOp> {
public:
  CmpIOp() : Op<CmpIOp>(0) {}
   explicit CmpIOp(Operation *op) : Op<CmpIOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static CmpIOp create(OpBuilder &builder, Location loc, CmpIPredicate predicate,
                       Value lhs, Value rhs);
};

} // namespace arith
} // namespace mlir
#endif
