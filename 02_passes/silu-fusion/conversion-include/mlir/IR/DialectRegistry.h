// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_DIALECTREGISTRY_H
#define BUDDY_MINI_MLIR_IR_DIALECTREGISTRY_H

#include "mlir/IR/Dialect.h"
#include "mlir/Support/TypeID.h"

// Provenance: audited Reshape V2 mini, line 1709; see docs/PROVENANCE.md.
namespace mlir {
using DialectAllocatorFunction = std::function<Dialect *(MLIRContext *)>;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1710; see docs/PROVENANCE.md.
namespace mlir {
class DialectRegistry {
  using MapTy =
      std::map<std::string, std::pair<TypeID, DialectAllocatorFunction>,
               std::less<>>;

public:
  explicit DialectRegistry();
  DialectRegistry(const DialectRegistry &) = delete;
  DialectRegistry &operator=(const DialectRegistry &other) = delete;
  DialectRegistry(DialectRegistry &&) = default;
  DialectRegistry &operator=(DialectRegistry &&other) = default;
   
   



private:
  MapTy registry;
  // Unused preload-name vector and extension MapVector storage omitted.
};
} // namespace mlir

#endif
