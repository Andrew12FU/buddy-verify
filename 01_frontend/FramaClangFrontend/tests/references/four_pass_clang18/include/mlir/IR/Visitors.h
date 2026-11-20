#ifndef FC_MLIR_IR_VISITORS_H
#define FC_MLIR_IR_VISITORS_H

#include "llvm/Support/Compiler.h"
#include "mlir/Support/WalkResult.h"

namespace mlir {

/* 官方 Visitors.h 定义 WalkOrder 与 walk 工具；WalkResult 在 21.x 已移到
 * mlir/Support/WalkResult.h（这里 include 过来，保持旧包含关系可用）。 */
enum class WalkOrder { PreOrder, PostOrder };

} // namespace mlir
#endif
