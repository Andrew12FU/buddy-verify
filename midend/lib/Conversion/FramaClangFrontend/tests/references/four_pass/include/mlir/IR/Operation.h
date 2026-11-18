#ifndef FC_MLIR_IR_OPERATION_H
#define FC_MLIR_IR_OPERATION_H

#include "llvm/ADT/Twine.h"

#include "mlir/IR/Block.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/IR/Visitors.h"

namespace mlir {

/* 官方 Operation：索引/计数一律 unsigned，诊断返回 InFlightDiagnostic，
 * walk 返回 RetT（模型统一返回 WalkResult —— 调用点都丢弃返回值，等价）。 */
class Operation {
public:
  MLIRContext *getContext();
  Location getLoc();

  ResultRange getResults();
  OperandRange getOperands();
  bool use_empty();
  use_range getUses();

  unsigned getNumOperands();
  Value getOperand(unsigned idx);
  OpOperand &getOpOperand(unsigned idx);
  llvm::MutableArrayRef<OpOperand> getOpOperands();
  void setOperand(unsigned idx, Value value);

  unsigned getNumResults();
  OpResult getResult(unsigned idx);

  void erase();
  InFlightDiagnostic emitOpError(const Twine &message = {});

  template <WalkOrder Order = WalkOrder::PostOrder, typename FnT>
  WalkResult walk(FnT &&callback) {
    return WalkResult::advance();
  }

private:
  const void *impl;
};

} // namespace mlir
#endif
