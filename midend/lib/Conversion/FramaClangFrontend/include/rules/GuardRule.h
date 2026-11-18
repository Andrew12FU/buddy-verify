// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_GUARDRULE_H
#define BUDDY_FRAMAC_GUARDRULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 根据 profile 为完整 guard 包块，不拆分表达式，不移动外部局部变量。
/// Matmul 在块内以具名 bool 保存原条件，防止纯标量 guard 的块被转换器展开。
/// @param S 当前 if；profile 决定是 Reshape 窄匹配还是 Matmul/SiLU
/// 方法直属匹配。
/// @param R 共享上下文，提供 AST 父子关系及源码范围。
/// @return 无；未匹配、已包块或不在已审计范围时不登记修改。
void planGuardRewrite(clang::IfStmt *S, RewriteContext &R);

} // namespace buddy::framac
#endif
