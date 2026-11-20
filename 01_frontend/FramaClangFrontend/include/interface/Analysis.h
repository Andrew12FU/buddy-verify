// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_HEADER_PILOT_ANALYSIS_H
#define BUDDY_HEADER_PILOT_ANALYSIS_H
#include "interface/Probe.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
namespace buddy::header_pilot {
/// @param D 当前 AST 声明；@return Clang USR
/// 或可读名称回退值；只在固定版本内使用。
std::string identity(const clang::Decl *D);
/// @param AST 当前解析上下文；@param D 被观察的声明。
/// @return 文件、行列、USR、源码等，宏区间明确标记，不把展开位置当可复制拼写。
llvm::json::Object describeDecl(clang::ASTContext &AST, const clang::Decl *D);
/// @param AST 已完成解析的 TU；@param Result 追加主文件调用、类型和 override
/// 事实。
/// @return 无；仅观察主文件，类型/调用不能被误认为递归语义依赖闭包。
void collectInventory(clang::ASTContext &AST, ProbeResult &Result);
/// @param AST 当前 TU；@param Result 写入真实 LogicalResult/ArrayRef
/// 的有界切片。
/// @return 无；只支持指定的两个类，其他类型不自动生成模型。
void collectLibrarySlices(clang::ASTContext &AST, ProbeResult &Result);
} // namespace buddy::header_pilot
#endif
