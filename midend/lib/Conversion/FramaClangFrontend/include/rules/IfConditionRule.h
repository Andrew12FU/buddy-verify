// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_IFCONDITIONRULE_H
#define BUDDY_FRAMAC_IFCONDITIONRULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 展开 SiLU 的 if/else-if 条件声明，保留查询时机、分支优先级和作用域。
/// @param S 当前 if；只处理块直属、含条件声明的根 if，else-if 由根统一渲染。
/// @param R 共享上下文；宏、非平凡析构和跳转等前提不足时登记拒绝。
/// @return 无；一个根条件链产生一个替换，不与子 else-if 重复登记。
void planIfConditionRewrite(clang::IfStmt *S, RewriteContext &R);

} // namespace buddy::framac
#endif
