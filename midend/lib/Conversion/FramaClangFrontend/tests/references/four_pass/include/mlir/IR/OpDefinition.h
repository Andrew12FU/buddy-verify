#ifndef FC_MLIR_IR_OPDEFINITION_H
#define FC_MLIR_IR_OPDEFINITION_H

#include "llvm/ADT/PointerUnion.h"

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"

namespace mlir {

/* 官方 OpFoldResult : PointerUnion<Attribute, Value>，构造继承自基类。 */
class OpFoldResult : public llvm::PointerUnion<Attribute, Value> {
public:
  OpFoldResult();
  OpFoldResult(Attribute attr);
  OpFoldResult(Value value);
};

/* 官方 OpState：所有 op 的公共基类，转发到 Operation。
 * 官方没有 operator! / operator Value —— operator Value 在官方由
 * OneTypedResult trait 提供给单结果 op，模型统一放在 Op<> 上（等价）。 */
class OpState {
public:
  explicit OpState(Operation *state) : state(state) {}

  explicit operator bool() { return getOperation() != 0; }
  operator Operation *() const { return state; }

  Operation *getOperation() { return state; }
  MLIRContext *getContext() { return getOperation()->getContext(); }
  Location getLoc() { return state->getLoc(); }

  void erase() { state->erase(); }
  InFlightDiagnostic emitOpError(const Twine &message = {});

  template <WalkOrder Order = WalkOrder::PostOrder, typename FnT>
  WalkResult walk(FnT &&callback) {
    return WalkResult::advance();
  }

private:
  Operation *state;
};

/* 官方 Op<ConcreteType, Traits...>（模型按"只定义用到的"去掉 traits 包）。 */
template <typename ConcreteType> class Op : public OpState {
public:
  /* 官方没有默认构造；模型的 CastInfo::castFailed() / getDefiningOp<T>()
   * 需要 `T()`，故补一个空句柄构造。 */
  Op() : OpState(0) {}
  explicit Op(Operation *state) : OpState(state) {}

  Operation *operator->() { return getOperation(); }

  /* 官方由 OneTypedResult 提供单结果 op → Value 的隐式转换；
   * 模型在多结果 op 上也会成立，但四个 pass 不会这样用。 */
  operator Value() { return getOperation()->getResult(0); }
};

} // namespace mlir
#endif
