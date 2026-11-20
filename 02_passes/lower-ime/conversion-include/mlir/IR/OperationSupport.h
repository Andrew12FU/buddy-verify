// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_OPERATIONSUPPORT_H
#define BUDDY_MINI_MLIR_IR_OPERATIONSUPPORT_H

#include "mlir/IR/ValueRange.h"

// Provenance: audited Reshape V2 mini, line 1061; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
class IROperandBase {
public:
   
protected:
  inline IROperandBase(Operation *owner);
  inline IROperandBase(IROperandBase &&other);
  inline IROperandBase &operator=(IROperandBase &&other);
  IROperandBase(const IROperandBase &use) = delete;
  IROperandBase &operator=(const IROperandBase &use) = delete;
  inline ~IROperandBase();
  IROperandBase *nextUse = nullptr;
  IROperandBase **back = nullptr;

private:
  Operation *const owner;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1075; see docs/PROVENANCE.md.
namespace mlir {
template <typename DerivedT, typename IRValueT>
class IROperand : public detail::IROperandBase {
public:
  inline IROperand(Operation *owner);
  inline IROperand(Operation *owner, IRValueT value);
  inline IROperand(IROperand &&other);
  inline IROperand &operator=(IROperand &&other);
   
   

private:
  IRValueT value = {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1086; see docs/PROVENANCE.md.
namespace mlir {
class OpOperand : public IROperand<OpOperand, Value> {
public:
   

private:
  friend class OperandStorage;
  using IROperand<OpOperand, Value>::IROperand;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1093; see docs/PROVENANCE.md.
namespace mlir {
class Location;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1094; see docs/PROVENANCE.md.
namespace mlir {
class OperationName;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1095; see docs/PROVENANCE.md.
namespace mlir {
class DictionaryAttr;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1096; see docs/PROVENANCE.md.
namespace mlir {
class PropertyRef;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1098; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
class alignas(8) OperandStorage {
public:
  OperandStorage(Operation *owner, OpOperand *trailingOperands,
                 ValueRange values);
  ~OperandStorage();
  // Only alignment is needed by the real TrailingObjects base computation.
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1104; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
enum class OpProperties : char {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1106; see docs/PROVENANCE.md.
namespace mlir {
class BlockOperand : public IROperand<BlockOperand, Block *> {
public:
  using IROperand<BlockOperand, Block *>::IROperand;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1530; see docs/PROVENANCE.md.
namespace mlir {
class OperationName {
public:
  class Impl;
  OperationName(StringRef name, MLIRContext *context);

protected:
  OperationName(Impl *impl);

private:
  Impl *impl = nullptr;
};
} // namespace mlir

#endif
