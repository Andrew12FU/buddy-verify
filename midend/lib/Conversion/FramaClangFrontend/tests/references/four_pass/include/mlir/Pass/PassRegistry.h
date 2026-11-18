#ifndef FC_MLIR_PASS_PASSREGISTRY_H
#define FC_MLIR_PASS_PASSREGISTRY_H

#include "mlir/Support/LLVM.h"

namespace mlir {

/* 官方 PassRegistration 在 mlir/Pass/PassRegistry.h（不在 Pass.h）。 */
template <typename PassT> class PassRegistration {
public:
  PassRegistration();

private:
  int reserved;
};

} // namespace mlir
#endif
