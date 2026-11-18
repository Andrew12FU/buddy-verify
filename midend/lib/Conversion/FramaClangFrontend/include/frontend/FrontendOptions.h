// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_FRONTEND_OPTIONS_H
#define BUDDY_FRAMAC_FRONTEND_OPTIONS_H

#include "llvm/ADT/StringRef.h"
#include <set>
#include <string>

namespace buddy::framac {
/// 一次改写的只读配置；由命令行入口创建，规则层不依赖全局命令行变量。
/// Profile 选择已审计的匹配范围，Rules 选择本次实际启用的规则。
struct FrontendOptions {
  std::string Profile = "reshape";
  std::string NamespacePrefix; ///< 空串表示使用输入文件名作为具名空间前缀。
  std::string InterfaceHeader; ///< 空串表示不进行实验性 include 替换。
  std::set<std::string> Rules;

  /// @param Name 规则标识，如 "capture"。
  /// @return 本次是否启用该规则；不表示该规则一定匹配或改写成功。
  bool enabled(llvm::StringRef Name) const;
};

/// @param Name profile 的命令行名称。
/// @return 是否为当前已接入的 pass；新增 pass 时在此登记。
bool isSupportedProfile(llvm::StringRef Name);

/// @param Name 独立规则标识。
/// @return 是否是工具认识的规则，而非某个 pass 必须启用的规则。
bool isKnownRule(llvm::StringRef Name);

/// 判断哪些真实 include 可在解析完成后被 mini 替代。
/// @param Profile 已审计 pass 的名称。
/// @param Header include 指令中的文件名，不含引号或尖括号。
/// @return 是否在该 pass 的白名单中；无关头文件必须保留。
bool isInterfaceInclude(llvm::StringRef Profile, llvm::StringRef Header);
} // namespace buddy::framac
#endif
