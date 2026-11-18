// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "support/AstSupport.h"
namespace buddy::framac {
/// 为已审计的同步 affine 回调规划引用捕获适配。
/// @param L 当前主文件中的 lambda，借用当前 AST。
/// @param R 保存指针声明、闭包替换和拒绝原因的上下文。
/// @return 无。只登记编辑；保留引用对象及成员的原存储，不复制 rewriter，
/// 不把成员改为全局变量。未知 this 用法、宏和嵌套闭包拒绝。
void planBorrowedCaptureRewrite(clang::LambdaExpr *L, RewriteContext &R);
} // namespace buddy::framac
