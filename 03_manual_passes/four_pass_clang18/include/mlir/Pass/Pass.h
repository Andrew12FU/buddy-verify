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

// 官方是 Pass::Option<DataType, OptionParser> 嵌套类；Frama-Clang 对
// 嵌套类模板 + 成员初始化列表 会断言失败（Clang_utils.h:892），故改为
// 命名空间级类模板并在此前置声明（参数/返回类型与官方一致）。
template <typename DataType, typename OptionParser = void> class Option;

/* 官方 Pass：基类，runOnOperation 是 protected 纯虚。 */
class Pass {
public:
  Pass();
  virtual ~Pass();

  virtual StringRef getArgument() const;
  virtual StringRef getDescription() const;
  virtual LogicalResult initialize(MLIRContext *context);
  virtual void getDependentDialects(DialectRegistry &registry) const;

  MLIRContext &getContext();
  void signalPassFailure();
  void markAllAnalysesPreserved();

protected:
  virtual void runOnOperation() = 0;
  Operation *getOperation();

private:
  void *impl;
};

/* 官方 `Pass::Option<DataType, OptionParser>`（嵌套类，变参构造把
 * cl::desc / cl::init 原样转发给底层的 llvm::cl::opt）。 */
template <typename DataType, typename OptionParser>
class Option {
public:
  /* 官方是变参模板 `Option(Pass&, StringRef, Args&&...)`；Frama-Clang 的
   * forward-reference 机制在变参构造上会断言失败，这里按 pass 唯一用到的
   * 四参形态（parent, arg, cl::desc, cl::init）固定下来。 */
  template <typename InitTy>
  Option(Pass &parent, StringRef arg, llvm::cl::desc description,
         llvm::cl::initializer<InitTy> initial);

  operator DataType() const;

  Option<DataType, OptionParser> &operator=(const DataType &value);

private:
  int reserved;
};

/* 官方 OperationPass<OpT>：public Pass，getOperation 返回具体 op。 */
template <typename OpT = void> class OperationPass : public Pass {
public:
  OpT getOperation();

protected:
  OperationPass();
};

template <> class OperationPass<void> : public Pass {
public:
  Operation *getOperation();

protected:
  OperationPass();
};

/* 官方 PassWrapper<PassT, BaseT>：第二参是**基类**（如 OperationPass<Op>）。 */
template <typename PassT, typename BaseT> class PassWrapper : public BaseT {
public:
  StringRef getName() const;
};

} // namespace mlir
#endif
