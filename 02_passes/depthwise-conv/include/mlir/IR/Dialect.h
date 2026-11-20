// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_DIALECT_H
#define BUDDY_MINI_MLIR_IR_DIALECT_H

#include "mlir/Support/LLVM.h"

// Provenance: audited Reshape V2 mini, line 1435; see docs/PROVENANCE.md.
namespace mlir {
class TypeID;
class Dialect {
public:
  virtual ~Dialect();

protected:
  Dialect(StringRef name, MLIRContext *context, TypeID id);

private:
  Dialect(const Dialect &) = delete;
  void operator=(Dialect &) = delete;
  // Real dialect tables are outside current registration declarations.
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1708; see docs/PROVENANCE.md.
namespace mlir {
class Dialect;
} // namespace mlir

#endif
