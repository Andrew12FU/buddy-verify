#ifndef FC_MLIR_IR_AFFINEEXPR_H
#define FC_MLIR_IR_AFFINEEXPR_H

#include "llvm/Support/Compiler.h"

namespace mlir {

/* Affine expression handle.  Only the arithmetic the passes spell out is
 * modelled: scaling by a constant, addition, modulo and floor division. */
class AffineExpr {
public:
  int reserved;

  AffineExpr();

  AffineExpr operator*(int64_t rhs) const;
  AffineExpr operator+(const AffineExpr &rhs) const;
  AffineExpr operator%(int64_t rhs) const;
  AffineExpr floorDiv(int64_t rhs) const;
};

} // namespace mlir
#endif
