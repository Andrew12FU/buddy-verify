// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_PATTERNMATCH_H
#define BUDDY_MINI_MLIR_IR_PATTERNMATCH_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpDefinition.h"

// Provenance: audited Reshape V2 mini, line 883; see docs/PROVENANCE.md.
namespace mlir {
class PatternRewriter;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1484; see docs/PROVENANCE.md.
namespace mlir {
class RewriterBase : public OpBuilder {
public:
  struct Listener;
  virtual void eraseOp(Operation *op);
   

  virtual void replaceOp(Operation *op, ValueRange newValues);
  virtual void replaceOp(Operation *op, Operation *newOp);
  inline virtual void startOpModification(Operation *op);
  virtual void finalizeOpModification(Operation *op);

  // A definition is necessary for instantiation with a local lambda type.
  // This is the exact used wrapper body from PatternMatch.h; notification
  // methods remain declarations, with no modeled IR behavior or contracts.
   






protected:
  inline explicit RewriterBase(MLIRContext *ctx,
                               OpBuilder::Listener *listener = nullptr);
  inline explicit RewriterBase(const OpBuilder &otherBuilder);
  inline explicit RewriterBase(Operation *op,
                               OpBuilder::Listener *listener = nullptr);
  virtual ~RewriterBase();

private:
  void operator=(const RewriterBase &) = delete;
  RewriterBase(const RewriterBase &) = delete;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1512; see docs/PROVENANCE.md.
namespace mlir {
 





} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1519; see docs/PROVENANCE.md.
namespace mlir {
class PatternBenefit {
  enum { ImpossibleToMatchSentinel = 65535 };

public:
  PatternBenefit() = default;
  PatternBenefit(unsigned benefit);
  PatternBenefit(const PatternBenefit &) = default;
  PatternBenefit &operator=(const PatternBenefit &) = default;

private:
  unsigned short representation{ImpossibleToMatchSentinel};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1540; see docs/PROVENANCE.md.
 
// Provenance: audited Reshape V2 mini, line 1542; see docs/PROVENANCE.md.
namespace mlir {
 















} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1558; see docs/PROVENANCE.md.
namespace mlir {
 











} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1571; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
 







} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1581; see docs/PROVENANCE.md.
namespace mlir {
 






} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1633; see docs/PROVENANCE.md.
namespace mlir {
template <typename OpTy> class OwningOpRef {
public:
  using OperationT = OpTy;
  OwningOpRef(std::nullptr_t = nullptr);
  OwningOpRef(OpTy op);
  OwningOpRef(OwningOpRef &&other);
  ~OwningOpRef();
  OwningOpRef &operator=(OwningOpRef &&other);

private:
  OpTy op;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1647; see docs/PROVENANCE.md.
namespace mlir {
 









} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1656; see docs/PROVENANCE.md.
namespace mlir {
 
























} // namespace mlir

#endif
