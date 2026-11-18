// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_TRANSFORMS_GREEDYPATTERNREWRITEDRIVER_H
#define BUDDY_MINI_MLIR_TRANSFORMS_GREEDYPATTERNREWRITEDRIVER_H

#include "mlir/Rewrite/FrozenRewritePatternSet.h"

// Provenance: audited Reshape V2 mini, line 1589; see docs/PROVENANCE.md.
namespace mlir {
enum class GreedyRewriteStrictness { AnyOp, ExistingAndNewOps, ExistingOps };
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1590; see docs/PROVENANCE.md.
namespace mlir {
enum class GreedySimplifyRegionLevel { Disabled, Normal, Aggressive };
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1591; see docs/PROVENANCE.md.
namespace mlir {
class GreedyRewriteConfig {
public:
  GreedyRewriteConfig &setStrictness(GreedyRewriteStrictness mode) {
    strictness = mode;
    return *this;
  }
  GreedyRewriteConfig &enableConstantCSE(bool enable = true) {
    cseConstants = enable;
    return *this;
  }
  GreedyRewriteConfig &enableFolding(bool enable = true) {
    fold = enable;
    return *this;
  }
  GreedyRewriteConfig &
  setRegionSimplificationLevel(GreedySimplifyRegionLevel level) {
    regionSimplificationLevel = level;
    return *this;
  }
  static constexpr long kNoLimit = -1;

private:
  Region *scope = nullptr;
  bool useTopDownTraversal = false;
  GreedySimplifyRegionLevel regionSimplificationLevel =
      GreedySimplifyRegionLevel::Aggressive;
  long maxIterations = 10;
  long maxNumRewrites = kNoLimit;
  GreedyRewriteStrictness strictness = GreedyRewriteStrictness::AnyOp;
  RewriterBase::Listener *listener = nullptr;
  bool fold = true;
  bool cseConstants = true;
  bool cseBetweenIterations = false;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1695; see docs/PROVENANCE.md.
namespace mlir {
LogicalResult
applyOpPatternsGreedily(ArrayRef<Operation *> ops,
                        const FrozenRewritePatternSet &patterns,
                        GreedyRewriteConfig config = GreedyRewriteConfig(),
                        bool *changed = nullptr, bool *allErased = nullptr);
} // namespace mlir

namespace mlir {
 



}
#endif
