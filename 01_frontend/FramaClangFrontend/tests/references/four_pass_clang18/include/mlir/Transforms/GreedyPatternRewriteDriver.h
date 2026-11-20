#ifndef FC_MLIR_TRANSFORMS_GREEDY_PATTERN_REWRITE_DRIVER_H
#define FC_MLIR_TRANSFORMS_GREEDY_PATTERN_REWRITE_DRIVER_H

#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/Region.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Support/LogicalResult.h"

namespace mlir {

/* 官方是 enum class，且有第三个取值 ExistingOps。 */
enum class GreedyRewriteStrictness { AnyOp, ExistingAndNewOps, ExistingOps };

/* 官方 21.x 的 GreedyRewriteConfig 是"私有字段 + 链式 setter"（setter 返回自身引用）。 */
class GreedyRewriteConfig {
public:
  GreedyRewriteConfig();

  GreedyRewriteStrictness getStrictness() const;
  GreedyRewriteConfig &setStrictness(GreedyRewriteStrictness mode);

  bool getUseTopDownTraversal() const;
  GreedyRewriteConfig &setUseTopDownTraversal(bool use = true);

  int64_t getMaxIterations() const;
  GreedyRewriteConfig &setMaxIterations(int64_t iterations);

private:
  GreedyRewriteStrictness strictness;
  bool useTopDownTraversal;
  int64_t maxIterations;
};

LogicalResult applyPatternsGreedily(Operation *op,
                                    const FrozenRewritePatternSet &patterns,
                                    GreedyRewriteConfig config = GreedyRewriteConfig(),
                                    bool *changed = 0);

LogicalResult applyPatternsGreedily(Region &region,
                                    const FrozenRewritePatternSet &patterns,
                                    GreedyRewriteConfig config = GreedyRewriteConfig(),
                                    bool *changed = 0);

} // namespace mlir
#endif
