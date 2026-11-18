// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_CAPTURERULE_H
#define BUDDY_FRAMAC_CAPTURERULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 将已审计的类对象引用捕获改为捕获原对象指针，并按声明身份替换引用。
/// @param L 当前 lambda，指针和子节点仅在本轮解析中有效。
/// @param R 共享上下文；只新增补丁和名字，不改变原对象或 AST。
/// SiLU profile 还支持单个自动局部变量的初始化；其他 profile 保持原范围。
/// 嵌套组由 NestedCapture 接管：逐层按值传播指针、组合为一个替换；
/// 组内允许单个自动变量初始化，其他单层声明初始化边界保持原样。
/// @return 无；部分候选即使已登记，出现拒绝后整份计划仍不会输出源码。
void planCaptureRewrite(clang::LambdaExpr *L, RewriteContext &R);

} // namespace buddy::framac
#endif
