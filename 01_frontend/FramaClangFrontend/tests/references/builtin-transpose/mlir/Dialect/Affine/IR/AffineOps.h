#ifndef MLIR_DIALECT_AFFINE_IR_AFFINEOPS_H
#define MLIR_DIALECT_AFFINE_IR_AFFINEOPS_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/IntegerSet.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/ValueRange.h"

#include <cstdint>

namespace mlir {
namespace affine {

class AffineDialect {};

class AffineApplyOp {
public:
  template <typename... Args>
  static Value create(Args &&...) {
    return Value();
  }
};

class AffineParallelOp {
public:
  template <typename... Args>
  static AffineParallelOp create(Args &&...) {
    return AffineParallelOp();
  }

  Region &getRegion() {
    static Region region;
    return region;
  }
};

class AffineIfOp {
public:
  template <typename... Args>
  static AffineIfOp create(Args &&...) {
    return AffineIfOp();
  }

  OpBuilder getThenBodyBuilder() const { return OpBuilder(); }
};

class AffineYieldOp {
public:
  template <typename... Args>
  static void create(Args &&...) {}
};

template <typename F>
void buildAffineLoopNest(OpBuilder &, Location, ValueRange, ValueRange, int64_t,
                         F) {}

} // namespace affine
} // namespace mlir

#endif
