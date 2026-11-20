#ifndef FC_MLIR_TRANSFORMS_DIALECTCONVERSION_H
#define FC_MLIR_TRANSFORMS_DIALECTCONVERSION_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Support/LogicalResult.h"

namespace mlir {

/* 官方 ConversionConfig（21.x 起 apply*Conversion 的第 4 参）。 */
struct ConversionConfig {
  ConversionConfig();

private:
  int reserved;
};

/* 官方 ConversionPattern / ConversionPatternRewriter 定义在 DialectConversion.h
 * （不在 PatternMatch.h）。 */
class ConversionPatternRewriter;

class ConversionPattern : public RewritePattern {
public:
  ConversionPattern(StringRef rootName, PatternBenefit benefit,
                    MLIRContext *context,
                    ArrayRef<StringRef> generatedNames = ArrayRef<StringRef>());

  virtual LogicalResult matchAndRewrite(Operation *op,
                                        ArrayRef<Value> operands,
                                        ConversionPatternRewriter &rewriter) const;
};

class ConversionPatternRewriter : public PatternRewriter {
public:
  ConversionPatternRewriter(MLIRContext *context,
                            const ConversionConfig &config);
};

/* 官方 ConversionTarget。 */
class ConversionTarget {
public:
  ConversionTarget(MLIRContext &ctx);
  virtual ~ConversionTarget();

  template <typename... Dialects> ConversionTarget &addLegalDialect() {
    return *this;
  }

  template <typename... Ops> ConversionTarget &addLegalOp() { return *this; }

  template <typename OpT> void addIllegalOp() {}

  template <typename OpT> void addDynamicallyLegalOp() {}

private:
  void *impl;
};

LogicalResult applyPartialConversion(Operation *op,
                                     const ConversionTarget &target,
                                     const FrozenRewritePatternSet &patterns,
                                     ConversionConfig config = ConversionConfig());

LogicalResult applyFullConversion(Operation *op,
                                  const ConversionTarget &target,
                                  const FrozenRewritePatternSet &patterns);

} // namespace mlir
#endif
