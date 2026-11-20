// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_REWRITE_PIPELINE_H
#define BUDDY_FRAMAC_REWRITE_PIPELINE_H
#include "frontend/FrontendOptions.h"
#include "rewrite/RewritePlan.h"
namespace clang::tooling {
class CompilationDatabase;
}
namespace buddy::framac {
/// 唯一的 Clang 调度入口：预处理收集 include 补丁，解析后两遍遍历 AST。
/// @param Compilations 调用方提供的编译数据库或 -- 后显式编译参数。
/// @param Input 原始 C++ 主文件路径，使用真实依赖完成解析。
/// @param Options 只读改写配置，其生命周期需覆盖整个调用。
/// @param Plan 输出原文、候选编辑、拒绝原因及 Parsed 标记。
/// @return ClangTool 的退出码；0 只表示前端执行成功，规则仍可能拒绝计划。
/// @note 不调用 finish，不写产物，也不运行 Frama-Clang。
int parseAndPlan(clang::tooling::CompilationDatabase &Compilations,
                 const std::string &Input, const FrontendOptions &Options,
                 RewritePlan &Plan);
} // namespace buddy::framac
#endif
