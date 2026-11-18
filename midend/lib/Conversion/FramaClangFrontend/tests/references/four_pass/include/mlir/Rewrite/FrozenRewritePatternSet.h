#ifndef FC_MLIR_REWRITE_FROZENREWRITEPATTERNSET_H
#define FC_MLIR_REWRITE_FROZENREWRITEPATTERNSET_H

#include "mlir/IR/PatternMatch.h"

namespace mlir {

/* 官方 FrozenRewritePatternSet：把 RewritePatternSet 冻结成不可变集合。
 * 官方构造吃 `RewritePatternSet &&`；模型的 std::move 替身返回左值引用，
 * 故这里收 `RewritePatternSet &`（见 include/FORCED_DEVIATIONS.md）。 */
class FrozenRewritePatternSet {
public:
  FrozenRewritePatternSet();
  FrozenRewritePatternSet(RewritePatternSet &patterns);

private:
  void *impl;
};

} // namespace mlir
#endif
