// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "support/AstSupport.h"
namespace buddy::framac {
/// Plan array-backed list arguments for one direct block child. Nested lambda
/// bodies are handled as their own statements. S/AST are borrowed; R
/// accumulates edits and rejection reasons. No arrays escape the enclosing
/// call's block. Transpose 的嵌套列表从叶节点分轮处理，每轮重新解析 AST；
/// 只允许有界的不可变元数据查询，所有父路径均不能是条件求值。
void planInitializerListRewrite(clang::Stmt *S, RewriteContext &R);
} // namespace buddy::framac
