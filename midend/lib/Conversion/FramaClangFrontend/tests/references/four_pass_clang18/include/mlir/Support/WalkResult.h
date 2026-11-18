#ifndef FC_MLIR_SUPPORT_WALKRESULT_H
#define FC_MLIR_SUPPORT_WALKRESULT_H

#include "mlir/Support/LogicalResult.h"

namespace mlir {

/* 官方 21.x 把 WalkResult 从 IR/Visitors.h 移到这里。
 * 注意官方没有 operator bool —— 判断用 wasInterrupted()/wasSkipped()。 */
class WalkResult {
  enum ResultEnum { Interrupt, Advance, Skip } result;

public:
  WalkResult(ResultEnum result = Advance) : result(result) {}

  WalkResult(LogicalResult result);

  bool operator==(const WalkResult &rhs) const { return result == rhs.result; }
  bool operator!=(const WalkResult &rhs) const { return result != rhs.result; }

  static WalkResult interrupt() { return WalkResult(Interrupt); }
  static WalkResult advance() { return WalkResult(Advance); }
  static WalkResult skip() { return WalkResult(Skip); }

  bool wasInterrupted() const { return result == Interrupt; }
  bool wasSkipped() const { return result == Skip; }
};

} // namespace mlir
#endif