// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_WHILERULE_H
#define BUDDY_FRAMAC_WHILERULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 将受限的 while 条件声明改成每轮体内声明，保留原 body 的内层作用域。
/// @param W 当前 while 节点，仅借用，绝不直接修改 AST。
/// @param R 共享配置、源码工具和计划。
/// @return 无；未匹配则跳过，已匹配但前提不足则向计划登记拒绝。
void planWhileRewrite(clang::WhileStmt *W, RewriteContext &R);

} // namespace buddy::framac
#endif
