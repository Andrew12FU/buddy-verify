// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_IFCONDITIONRULE_H
#define BUDDY_FRAMAC_IFCONDITIONRULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 统一规范化并展开条件声明，保留查询时机、分支优先级和作用域。
/// @param S 当前 if；只处理块直属、含条件声明的根 if，else-if 由根统一渲染。
/// @param R 共享上下文；宏、非平凡析构和跳转等前提不足时登记拒绝。
/// @return 无；每个根条件链本轮只规范化或展开之一。调用方须重新解析并重复
/// 到零编辑；一般嵌套延后处理，else-if 由根接管，避免重叠补丁。
void planIfConditionRewrite(clang::IfStmt *S, RewriteContext &R);

} // namespace buddy::framac
#endif
