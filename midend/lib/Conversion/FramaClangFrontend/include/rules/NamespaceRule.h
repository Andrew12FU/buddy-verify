// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_NAMESPACERULE_H
#define BUDDY_FRAMAC_NAMESPACERULE_H

#include "support/AstSupport.h"
#include "clang/AST/TypeLoc.h"
#include <map>

namespace buddy::framac {
/// 有状态的命名空间规则：同一匿名空间的各次重开必须共享新名字。
class NamespaceRule {
  RewriteContext &R;
  std::map<const clang::NamespaceDecl *, std::string> NamespaceNames;

public:
  /// @param R 借用当前翻译单元上下文；构造时确定并保留具名空间的新名字。
  explicit NamespaceRule(RewriteContext &R);
  /// @param D 当前访问的命名空间声明；未启用或非匿名时不做任何修改。
  /// @return 无；候选修改与拒绝原因统一写入 R.Plan。
  void planDeclaration(clang::NamespaceDecl *D);
  /// @param T 原文类类型的使用位置；用声明身份找到应限定的外部引用。
  /// @return 无；内部宏引用保留，外部不可编辑宏引用导致整份计划拒绝。
  void planTypeReference(clang::RecordTypeLoc T);
};

} // namespace buddy::framac
#endif
