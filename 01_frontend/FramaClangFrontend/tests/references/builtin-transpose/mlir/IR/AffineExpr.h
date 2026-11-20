#ifndef MLIR_IR_AFFINEEXPR_H
#define MLIR_IR_AFFINEEXPR_H

#include <cstdint>

namespace mlir {

class AffineExpr {
public:
  AffineExpr floorDiv(int64_t) const { return AffineExpr(); }
};

inline AffineExpr operator%(AffineExpr, int64_t) { return AffineExpr(); }
inline AffineExpr operator*(AffineExpr, int64_t) { return AffineExpr(); }
inline AffineExpr operator-(AffineExpr, int64_t) { return AffineExpr(); }

} // namespace mlir

#endif
