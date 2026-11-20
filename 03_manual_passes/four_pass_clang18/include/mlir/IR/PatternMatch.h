#ifndef FC_MLIR_IR_PATTERNMATCH_H
#define FC_MLIR_IR_PATTERNMATCH_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"

namespace mlir {

/* 官方 PatternBenefit（数值越小优先级越高，只是包装一个 unsigned）。 */
class PatternBenefit {
public:
  PatternBenefit();
  PatternBenefit(unsigned benefit) : value(benefit) {}

  unsigned getBenefit() const { return value; }

private:
  unsigned value;
};

/* 官方 RewritePattern 基类。 */
class RewritePattern {
public:
  virtual ~RewritePattern();

private:
  /* 官方存 benefit / rootKind / generatedNames；这里只留一个字段，
   * 同时避免生成 C 结构体为空（Frama-C 默认 machdep 不接受空结构体）。 */
  PatternBenefit benefit;
};

class PatternRewriter;

/* 官方 OpRewritePattern<SourceOp>：构造带默认 benefit。 */
template <typename SourceOp> class OpRewritePattern : public RewritePattern {
public:
  OpRewritePattern(MLIRContext *context, PatternBenefit benefit = 1) {}

  virtual LogicalResult matchAndRewrite(SourceOp op,
                                        PatternRewriter &rewriter) const;
};

class RewritePatternSet {
public:
  explicit RewritePatternSet(MLIRContext *context);

  /* 官方是 `template <typename... Ts, typename... Args> RewritePatternSet &add(Args&&...)`。
   * Frama-Clang 对变参模板实参包会生成重名的形参（redefinition of 'args'），
   * 故按 pass 里实际用到的两种调用（add<T>(ctx) 与 add<T>(ctx, extra)）写死元数。 */
  template <typename T> RewritePatternSet &add();
  template <typename T, typename A0> RewritePatternSet &add(A0 arg0);
  template <typename T, typename A0, typename A1>
  RewritePatternSet &add(A0 arg0, A1 arg1);

private:
  void *impl;
};

/* 官方 PatternRewriter : OpBuilder。 */
class PatternRewriter : public OpBuilder {
public:
  explicit PatternRewriter(MLIRContext *ctx) : OpBuilder(ctx) {}

  virtual void eraseOp(Operation *op);
};

} // namespace mlir
#endif
