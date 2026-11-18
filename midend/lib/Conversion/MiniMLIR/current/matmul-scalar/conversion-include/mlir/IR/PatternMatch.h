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
  template <typename OpTy, typename... Args>
  OpTy replaceOpWithNewOp(Operation *op, Args &&...args);
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
class PatternRewriter : public RewriterBase {
public:
  inline explicit PatternRewriter(MLIRContext *ctx);
  using RewriterBase::RewriterBase;
  inline virtual bool canRecoverFromRewriteFailure() const;
};
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
template class llvm::SmallVector<mlir::OperationName, 2>;
// Provenance: audited Reshape V2 mini, line 1542; see docs/PROVENANCE.md.
namespace mlir {
class Pattern {
  enum class RootKind { Any, OperationName, InterfaceID, TraitID };

protected:
  Pattern(StringRef rootName, PatternBenefit benefit, MLIRContext *context,
          ArrayRef<StringRef> generatedNames = {});

private:
  const void *rootValue;
  RootKind rootKind;
  const PatternBenefit benefit;
  llvm::PointerIntPair<MLIRContext *, 1, bool> contextAndHasBoundedRecursion;
  SmallVector<OperationName, 2> generatedOps;
  StringRef debugName;
  SmallVector<StringRef, 0> debugLabels;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1558; see docs/PROVENANCE.md.
namespace mlir {
class RewritePattern : public Pattern {
public:
  virtual ~RewritePattern() = default;
  virtual LogicalResult matchAndRewrite(Operation *op,
                                        PatternRewriter &rewriter) const = 0;

protected:
  using Pattern::Pattern;

private:
  virtual void anchor();
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1571; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename SourceOp>
struct OpOrInterfaceRewritePatternBase : public RewritePattern {
  using RewritePattern::RewritePattern;
  inline LogicalResult matchAndRewrite(Operation *op,
                                       PatternRewriter &rewriter) const final;
  virtual LogicalResult matchAndRewrite(SourceOp op,
                                        PatternRewriter &rewriter) const = 0;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1581; see docs/PROVENANCE.md.
namespace mlir {
template <typename SourceOp>
struct OpRewritePattern
    : public mlir::detail::OpOrInterfaceRewritePatternBase<SourceOp> {
  using Base = OpRewritePattern;
  inline OpRewritePattern(MLIRContext *context, PatternBenefit benefit = 1,
                          ArrayRef<StringRef> generatedNames = {});
};
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
class PDLPatternModule {
public:
  PDLPatternModule() = default;
  PDLPatternModule(OwningOpRef<ModuleOp> module);
  template <typename... ConfigsT>
  PDLPatternModule(OwningOpRef<ModuleOp> module, ConfigsT &&...patternConfigs);

private:
  OwningOpRef<ModuleOp> pdlModule;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1656; see docs/PROVENANCE.md.
namespace mlir {
class RewritePatternSet {
  using NativePatternListT = std::vector<std::unique_ptr<RewritePattern>>;

public:
  inline RewritePatternSet(MLIRContext *context);
  // Real candidate declarations/constraint. addImpl/create/allocation bodies
  // remain external.
  template <typename... Ts, typename ConstructorArg,
            typename... ConstructorArgs,
            typename = std::enable_if_t<sizeof...(Ts) != 0>>
  inline RewritePatternSet &add(ConstructorArg &&arg,
                                ConstructorArgs &&...args);
  template <typename... Ts> inline RewritePatternSet &add();
  inline RewritePatternSet &add(std::unique_ptr<RewritePattern> pattern);
  inline RewritePatternSet &add(PDLPatternModule &&pattern);
  template <typename OpType>
  inline RewritePatternSet &
  add(LogicalResult (*implFn)(OpType, PatternRewriter &),
      PatternBenefit benefit = 1, ArrayRef<StringRef> generatedNames = {});

private:
  MLIRContext *const context;
  NativePatternListT nativePatterns;
  PDLPatternModule pdlPatterns;
};
} // namespace mlir

#endif
