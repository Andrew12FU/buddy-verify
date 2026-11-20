#ifndef FC_MLIR_IR_OPERATIONSUPPORT_H
#define FC_MLIR_IR_OPERATIONSUPPORT_H

#include "mlir/IR/Value.h"

/* 官方 OperationSupport.h 提供 OperationName / OperationState / NamedAttrList /
 * OpPrintingFlags / OperandStorage 等；四个 pass 都没用到，
 * 按"只定义用到的"这里不再声明内容（保留文件与包含关系，与官方头布局一致）。 */
namespace mlir {}

#endif
