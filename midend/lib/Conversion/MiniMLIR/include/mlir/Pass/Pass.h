// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_PASS_PASS_H
#define BUDDY_MINI_MLIR_PASS_PASS_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/Pass/PassOptions.h"

// Provenance: audited Reshape V2 mini, line 1701; see docs/PROVENANCE.md.
namespace mlir {
class Pass;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1702; see docs/PROVENANCE.md.
namespace mlir {
using PassAllocatorFunction = std::function<std::unique_ptr<Pass>()>;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1703; see docs/PROVENANCE.md.
namespace mlir {
template <typename ConcretePass> struct PassRegistration {
  inline PassRegistration(const PassAllocatorFunction &constructor);
  inline PassRegistration();
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1757; see docs/PROVENANCE.md.
namespace mlir {
class Pass {
public:
 template<typename DataType,typename OptionParser=llvm::cl::parser<DataType>>
 struct Option : public detail::PassOptions::Option<DataType,OptionParser> {
  using detail::PassOptions::Option<DataType,OptionParser>::operator=;
  template<typename... Args> Option(Pass &parent,StringRef arg,Args &&...args);
 };
  virtual ~Pass() = default;
  virtual StringRef getName() const = 0;
  inline virtual StringRef getArgument() const;
  inline virtual StringRef getDescription() const;
  inline virtual void getDependentDialects(DialectRegistry &registry) const;

protected:
  virtual std::unique_ptr<Pass> clonePass() const = 0;
  virtual void runOnOperation() = 0;
  MLIRContext &getContext();
  void signalPassFailure();
  inline Pass(const Pass &other);
  Pass &operator=(const Pass &) = delete;
  Pass(Pass &&) = delete;
  Pass &operator=(Pass &&) = delete;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1778; see docs/PROVENANCE.md.
namespace mlir {
template <typename OpT = void> class OperationPass : public Pass {
public:
  ~OperationPass() override = default;

protected:
  inline OpT getOperation();
  inline OperationPass(TypeID passID);
  OperationPass(const OperationPass &) = default;
  OperationPass &operator=(const OperationPass &) = delete;
  OperationPass(OperationPass &&) = delete;
  OperationPass &operator=(OperationPass &&) = delete;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1790; see docs/PROVENANCE.md.
namespace mlir {
template <typename PassT, typename BaseT> class PassWrapper : public BaseT {
public:
  ~PassWrapper() override = default;

protected:
  inline PassWrapper();
  PassWrapper(const PassWrapper &) = default;
  PassWrapper &operator=(const PassWrapper &) = delete;
  PassWrapper(PassWrapper &&) = delete;
  PassWrapper &operator=(PassWrapper &&) = delete;
  inline StringRef getName() const override;
  inline std::unique_ptr<Pass> clonePass() const override;
};
} // namespace mlir

#endif
