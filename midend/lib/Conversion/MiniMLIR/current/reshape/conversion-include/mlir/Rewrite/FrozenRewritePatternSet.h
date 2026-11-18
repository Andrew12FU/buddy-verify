// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_REWRITE_FROZENREWRITEPATTERNSET_H
#define BUDDY_MINI_MLIR_REWRITE_FROZENREWRITEPATTERNSET_H

#include "mlir/IR/PatternMatch.h"

// Provenance: audited Reshape V2 mini, line 1678; see docs/PROVENANCE.md.
namespace mlir {
class FrozenRewritePatternSet {
public:
  FrozenRewritePatternSet();
  FrozenRewritePatternSet(FrozenRewritePatternSet &&patterns) = default;
  FrozenRewritePatternSet(const FrozenRewritePatternSet &patterns) = default;
  FrozenRewritePatternSet &
  operator=(const FrozenRewritePatternSet &patterns) = default;
  FrozenRewritePatternSet &
  operator=(FrozenRewritePatternSet &&patterns) = default;
  FrozenRewritePatternSet(RewritePatternSet &&patterns,
                          ArrayRef<std::string> disabledPatternLabels = {},
                          ArrayRef<std::string> enabledPatternLabels = {});
  ~FrozenRewritePatternSet();

private:
  struct Impl;
  std::shared_ptr<Impl> impl;
};
} // namespace mlir

#endif
