// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_FRONTEND_OUTPUT_H
#define BUDDY_FRAMAC_FRONTEND_OUTPUT_H
#include "frontend/FrontendOptions.h"
#include "rewrite/RewritePlan.h"
namespace buddy::framac {
/// @param A 第一个路径，可以尚不存在。
/// @param B 第二个路径；现存文件还检查 inode/符号链接。
/// @return 路径解析后相同或指向同一现存文件时为 true，供入口保护原文件。
bool samePath(llvm::StringRef A, llvm::StringRef B);

/// @param Path 目标文件，父目录需已存在；调用前入口必须检查禁止覆盖条件。
/// @param Contents 完整文件内容。
/// @return 写入并关闭成功时为 true；失败向 stderr 输出原因并返回 false。
/// @note 本函数不做原子发布或跨文件事务，不能脱离入口的保护直接调用。
bool writeFile(llvm::StringRef Path, llvm::StringRef Contents);

/// @param Plan 已经调用 finish 的计划，包含排好序的原始字节范围。
/// @param Options 仅用于报告本次 profile/rules，不重新执行任何规则。
/// @param OutputPath 报告中声明的输出路径。
/// @param Accepted 内存计划是否接受；不代表文件落盘或 Frama 转换成功。
/// @return schema_version=1 的 JSON 文本，保留原有字段和顺序无关的语义。
std::string serializeReport(const RewritePlan &Plan,
                            const FrontendOptions &Options,
                            llvm::StringRef OutputPath, bool Accepted);
} // namespace buddy::framac
#endif
