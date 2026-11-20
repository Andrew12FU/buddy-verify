#ifndef FC_MLIR_SUPPORT_LOGICALRESULT_H
#define FC_MLIR_SUPPORT_LOGICALRESULT_H

/* 官方 21.x 这个头是转发 stub（using llvm::LogicalResult 等）。
 * Frama-Clang 解析不了类型的 using 声明，故用 typedef + 同名转发函数。 */
#include "llvm/Support/LogicalResult.h"

namespace mlir {

typedef llvm::LogicalResult LogicalResult;

/* 只转发不带 LogicalResult 形参的两个（failure()/success()）。
 * `failed(x)` / `succeeded(x)` 走 ADL 直接命中 llvm 版本；若在这里也声明一份，
 * 会与 ADL 找到的 llvm::failed 重载冲突（ambiguous）。 */
LogicalResult success(bool isSuccess = true);
LogicalResult failure(bool isFailure = true);

} // namespace mlir
#endif
