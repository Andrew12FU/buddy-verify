#ifndef FC_MLIR_IR_DIAGNOSTICS_H
#define FC_MLIR_IR_DIAGNOSTICS_H

#include "llvm/ADT/Twine.h"

#include "mlir/Support/LLVM.h"

namespace mlir {

/* 官方 InFlightDiagnostic 是 move-only 的 RAII 句柄（带 operator<<）。
 * 模型只定义 pass 用到的"能被返回/被丢弃"这一最小形态。 */
class InFlightDiagnostic {
public:
  InFlightDiagnostic();

private:
  const void *impl;
};

} // namespace mlir
#endif
