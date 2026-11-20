// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_HEADER_EXTRACTION_PILOT_H
#define BUDDY_HEADER_EXTRACTION_PILOT_H
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/JSON.h"
#include <memory>
#include <string>

namespace buddy::header_pilot {
/// 单翻译单元的持久化结果，只存文本及来源，不保存 AST 指针。
struct ProbeResult {
  llvm::json::Array Calls, Types, Overrides, Includes, Macros;
  llvm::json::Object Library;
  bool Parsed = false;
};
/// 创建独立的 Clang action 工厂。
/// @param Result 由调用者持有，生命周期覆盖 ClangTool::run；只追加观察结果。
/// @return 工厂的独占所有权。结果不构成完整接口闭包或语义契约。
std::unique_ptr<clang::tooling::FrontendActionFactory>
makeProbeFactory(ProbeResult &Result);
/// @param Result 已收集的单文件结果；@param Input 原输入名称。
/// @return JSON 文本，包含解析状态、实际调用、类型性质和有界源码切片。
std::string serialize(ProbeResult &Result, const std::string &Input);
} // namespace buddy::header_pilot
#endif
