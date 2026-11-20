// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_CAPTURERULE_H
#define BUDDY_FRAMAC_CAPTURERULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 按 AST 结构把引用捕获改为原对象指针的值捕获，普通值捕获保留复制语义。
/// @param L 当前 lambda，指针和子节点仅在本轮解析中有效。
/// @param R 共享上下文；只新增补丁和名字，不改变原对象或 AST。
/// 所有 profile 共用单层/嵌套捕获树规划；支持同块表达式及单个自动变量初始化。
/// this 仅支持普通非 volatile 整数字段访问，改为捕获对应实例的成员地址。
/// 不延长被引用对象寿命；初始化捕获等未支持结构明确拒绝。
/// @return 无；部分候选即使已登记，出现拒绝后整份计划仍不会输出源码。
void planCaptureRewrite(clang::LambdaExpr *L, RewriteContext &R);

} // namespace buddy::framac
#endif
