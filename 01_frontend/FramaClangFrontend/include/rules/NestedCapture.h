// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_NESTED_CAPTURE_H
#define BUDDY_FRAMAC_NESTED_CAPTURE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 统一的捕获树规划入口；单层 lambda 也是只有一个节点的树。
/// @param L 当前 lambda。最外层节点负责整个捕获树，内层回调不重复登记。
/// @param R 借用的 AST、源文本和计划；只登记原对象指针及组合后的文本补丁。
/// @return 无；内层回调不重复提交全局补丁。
/// @note 支持对象、标量、引用变量的引用捕获，保留普通值捕获；支持受限 this
/// 字段访问。初始化捕获、跨已改写引用的值捕获、泛型、宏和未求值使用拒绝。
/// 每层先复制为体内局部指针，再按值捕获，不延长原对象寿命，不证明回调逃逸后的使用有效。
void planCaptureTreeRewrite(clang::LambdaExpr *L, RewriteContext &R);
} // namespace buddy::framac
#endif
