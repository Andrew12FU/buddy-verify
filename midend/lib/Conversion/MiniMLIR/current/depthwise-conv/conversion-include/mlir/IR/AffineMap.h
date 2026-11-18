// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_IR_AFFINEMAP_H
#define BUDDY_MINI_MLIR_IR_AFFINEMAP_H
#include "mlir/IR/AffineExpr.h"
namespace mlir { namespace detail { struct AffineMapStorage; }
class AffineMap { detail::AffineMapStorage *map=nullptr;
public:
 static AffineMap get(unsigned dimCount,unsigned symbolCount,AffineExpr result); AffineMap() = default;
  // Restore the external declaration used by SiLU; no map-building algorithm
  // is introduced. Signature: local mlir/IR/AffineMap.h.
  static AffineMap get(unsigned dimCount, unsigned symbolCount,
                       ArrayRef<AffineExpr> results, MLIRContext *context);
}; }

#endif
