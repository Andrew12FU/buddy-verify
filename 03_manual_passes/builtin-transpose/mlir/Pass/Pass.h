#ifndef MLIR_PASS_PASS_H
#define MLIR_PASS_PASS_H

#include "llvm/Support/CommandLine.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/LLVM.h"

namespace mlir {

class Pass {
public:
  virtual ~Pass() = default;
  virtual StringRef getArgument() const = 0;
  virtual StringRef getDescription() const = 0;
  virtual void runOnOperation() {}
  virtual void getDependentDialects(DialectRegistry &) const {}

  void signalPassFailure() {}

  MLIRContext &getContext() const {
    static MLIRContext context;
    return context;
  }
};

template <typename OpTy>
class OperationPass : public Pass {
public:
  OpTy getOperation() const { return OpTy(); }
};

template <typename DerivedT, typename BaseT>
class PassWrapper : public BaseT {};

template <typename T>
class Option {
public:
  template <typename... Args>
  Option(Pass &, const char *, Args &&...) {}

  Option &operator=(T) { return *this; }
  operator T() const { return T(); }
};

template <typename PassT>
class PassRegistration {
public:
  PassRegistration() = default;
};

#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(className)

} // namespace mlir

#endif
