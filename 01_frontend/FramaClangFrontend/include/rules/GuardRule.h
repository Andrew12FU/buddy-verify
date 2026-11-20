// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_GUARDRULE_H
#define BUDDY_FRAMAC_GUARDRULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 按 if + 内建 || 的 AST 结构统一改写，不检查 profile、类名、方法或返回值。
/// 块内 bool 保存完整原条件，完整保留 then/else；if-init 移入同一个外层块。
/// @param S 当前运行期 if；循环内、无块分支和 else-if 同样适用。
/// @param R 共享上下文，提供 AST 父子关系、源码范围及名字避让。
/// @return 无；嵌套候选留到下一轮 AST。宏和 goto/label 登记拒绝；
/// if constexpr 和重载 || 不属于这条运行期规则。
void planGuardRewrite(clang::IfStmt *S, RewriteContext &R);

} // namespace buddy::framac
#endif
