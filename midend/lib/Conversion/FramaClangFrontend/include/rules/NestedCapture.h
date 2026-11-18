// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_NESTED_CAPTURE_H
#define BUDDY_FRAMAC_NESTED_CAPTURE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 嵌套捕获的内部规划入口，由 CaptureRule 调用，不是独立 CLI 规则。
/// @param L 当前 lambda。最外层节点负责整个捕获树，内层回调不重复登记。
/// @param R 借用的 AST、源文本和计划；只登记原对象指针及组合后的文本补丁。
/// @return true 表示属于嵌套组，已规划、无须修改或已登记拒绝，调用者应返回；
/// false 表示是非嵌套 lambda，继续使用原来的单层规则。
/// @note 支持空捕获和类对象的引用捕获；复制/混合/this/init/reference-variable
/// 捕获、泛型、宏及未求值使用仍拒绝。捕获树中的自动单变量初始化允许放置指针。
/// 每层先复制为体内局部指针，再按值捕获，不延长原对象寿命，不证明回调逃逸后的使用有效。
bool planNestedCaptureRewrite(clang::LambdaExpr *L, RewriteContext &R);
} // namespace buddy::framac
#endif
