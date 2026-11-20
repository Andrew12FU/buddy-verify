#ifndef MLIR_TRANSFORMS_DIALECTCONVERSION_H
#define MLIR_TRANSFORMS_DIALECTCONVERSION_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Support/LogicalResult.h"

namespace mlir {

class ConversionPatternRewriter : public OpBuilder {
public:
  void eraseOp(Operation *) {}
  void setInsertionPointToStart(Block *) {}

  template <typename OpT>
  void setInsertionPointAfter(OpT) {}
};

class ConversionPattern {
public:
  ConversionPattern(StringRef, int, MLIRContext *) {}
  virtual ~ConversionPattern() = default;

  virtual LogicalResult matchAndRewrite(Operation *, ArrayRef<Value>,
                                        ConversionPatternRewriter &) const = 0;
};

class ConversionTarget {
public:
  explicit ConversionTarget(MLIRContext &) {}

  template <typename... Dialects>
  void addLegalDialect() {}

  template <typename... Ops>
  void addLegalOp() {}
};

class RewritePatternSet {
public:
  explicit RewritePatternSet(MLIRContext *) {}

  template <typename PatternT, typename... Args>
  void add(Args &&...) {}
};

template <typename... Args>
LogicalResult applyPartialConversion(Args &&...) {
  return success();
}

} // namespace mlir

#endif
