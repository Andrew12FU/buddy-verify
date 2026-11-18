#ifndef FC_MLIR_PASS_PASS_H
#define FC_MLIR_PASS_PASS_H

#include "llvm/Support/CommandLine.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Support/TypeID.h"

namespace mlir {

class RegisteredOperationName;

/* 官方是 `Pass::Option<DataType, OptionParser>` 嵌套类；Frama-Clang 对
 * 「嵌套类模板 + 成员初始化列表」会断言失败（Clang_utils.h:892），故改为
 * 命名空间级类模板（构造/转换运算符与官方一致，见 FORCED_DEVIATIONS）。 */
template <typename DataType, typename OptionParser = void> class Option;

/* 官方 Pass（23.x）：protected `explicit Pass(TypeID, optional<StringRef>)`，
 * runOnOperation 是 protected 纯虚；getName/canScheduleOn 也是纯虚。
 * 模型没有 <optional>，省掉可选 opName 形参。 */
class Pass {
public:
  virtual ~Pass();

  virtual StringRef getArgument() const;
  virtual StringRef getDescription() const;
  virtual StringRef getName() const = 0;
  virtual bool canScheduleOn(RegisteredOperationName opName) const = 0;
  virtual bool canScheduleOn(Operation *op) const;
  virtual void getDependentDialects(DialectRegistry &registry) const;

  void signalPassFailure();
  void markAllAnalysesPreserved();

protected:
  explicit Pass(TypeID passID);
  virtual LogicalResult initialize(MLIRContext *context);
  MLIRContext &getContext();
  Operation *getOperation();
  virtual void runOnOperation() = 0;

private:
  void *impl;
};

template <typename DataType, typename OptionParser>
class Option {
public:
  template <typename InitTy>
  Option(Pass &parent, StringRef arg, llvm::cl::desc description,
         llvm::cl::initializer<InitTy> initial);

  operator DataType() const;

  Option<DataType, OptionParser> &operator=(const DataType &value);

private:
  int reserved;
};

/* 官方 OperationPass<OpT>：public Pass，getOperation() 是 protected、返回 OpT。 */
template <typename OpT = void> class OperationPass : public Pass {
protected:
  explicit OperationPass(TypeID passID);

  OpT getOperation();
};

template <> class OperationPass<void> : public Pass {
protected:
  explicit OperationPass(TypeID passID);

  Operation *getOperation();
};

/* 官方 PassWrapper<PassT, BaseT>：第二参是基类；构造把 PassT 的 TypeID 传给基类。 */
template <typename PassT, typename BaseT> class PassWrapper : public BaseT {
public:
  PassWrapper() : BaseT(TypeID::get<PassT>()) {}

  StringRef getName() const;
  bool canScheduleOn(RegisteredOperationName opName) const;

  static bool classof(const Pass *pass);

private:
  int reserved;
};

} // namespace mlir
#endif