// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_IR_AFFINEEXPR_H
#define BUDDY_MINI_MLIR_IR_AFFINEEXPR_H
#include "mlir/Support/LLVM.h"

namespace mlir {
namespace detail {
struct AffineExprStorage;
}
class AffineExpr {
public:
  AffineExpr operator*(int64_t value) const;
  AffineExpr operator+(AffineExpr other) const;
  AffineExpr operator%(uint64_t value) const;
  AffineExpr floorDiv(uint64_t value) const;
   
  using ImplType = detail::AffineExprStorage;
  constexpr AffineExpr() {}

private:
  ImplType *expr;
};
} // namespace mlir
#endif
