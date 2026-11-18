#ifndef FC_MLIR_DIALECT_SCF_IR_SCF_H
#define FC_MLIR_DIALECT_SCF_IR_SCF_H

#include "llvm/ADT/STLFunctionalExtras.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {
namespace scf {

class SCFDialect : public Dialect {
public:
  SCFDialect();
};

class YieldOp : public Op<YieldOp> {
public:
  YieldOp() : Op<YieldOp>(0) {}
   explicit YieldOp(Operation *op) : Op<YieldOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static YieldOp create(OpBuilder &builder, Location loc, ValueRange values);
};

class ForOp : public Op<ForOp> {
public:
  ForOp() : Op<ForOp>(0) {}
   explicit ForOp(Operation *op) : Op<ForOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static ForOp create(OpBuilder &builder, Location loc, Value lowerBound,
                      Value upperBound, Value step, ValueRange iterArgs,
                      llvm::function_ref<void(OpBuilder &, Location, Value,
                                              ValueRange)> body);

  Value getInductionVar();
};

class ForallOp : public Op<ForallOp> {
public:
  ForallOp() : Op<ForallOp>(0) {}
   explicit ForallOp(Operation *op) : Op<ForallOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  /* 官方 mapping 形参是 std::optional<ArrayAttr>；模型前端没有 <optional>，
   * 这里收模型的 std::nullopt_t（唯一实际传值），见 FORCED_DEVIATIONS。 */
  static ForallOp create(OpBuilder &builder, Location loc,
                        llvm::ArrayRef<OpFoldResult> ubs, ValueRange outputs,
                        std::nullopt_t mapping,
                        llvm::function_ref<void(OpBuilder &, Location,
                                                ValueRange)> body);
};

class IfOp : public Op<IfOp> {
public:
  IfOp() : Op<IfOp>(0) {}
   explicit IfOp(Operation *op) : Op<IfOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static IfOp create(OpBuilder &builder, Location loc, Value cond,
                     llvm::function_ref<void(OpBuilder &, Location)> thenBuilder,
                     llvm::function_ref<void(OpBuilder &, Location)> elseBuilder =
                         llvm::function_ref<void(OpBuilder &, Location)>());
};

class InParallelOp : public Op<InParallelOp> {
public:
  InParallelOp() : Op<InParallelOp>(0) {}
   explicit InParallelOp(Operation *op) : Op<InParallelOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  static InParallelOp create(OpBuilder &builder, Location loc);
};

} // namespace scf
} // namespace mlir
#endif
