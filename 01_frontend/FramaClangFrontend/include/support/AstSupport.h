// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_AST_SUPPORT_H
#define BUDDY_FRAMAC_AST_SUPPORT_H

#include "frontend/FrontendOptions.h"
#include "rewrite/RewritePlan.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/SourceManager.h"
#include <set>
#include <string>
#include <vector>

namespace buddy::framac {
/// 第一遍 AST 扫描得到的主文件清单，供后续规则查名、生成新名字和定位匿名空间。
/// Names 持有字符串副本；Anonymous 仅借用 AST 节点地址，不拥有或复制节点。
/// 节点内存由对应的 ASTContext 管理；其销毁后，即使清单还在，节点指针也已
/// 悬空且不会自动变成 nullptr，不能再解引用，也不要自行 delete 这些节点。
struct MainFileInventory {
  /// 已有标识符，用于 freshName 避免新名字冲突；也记录本轮已生成的新名字。
  std::set<std::string> Names;
  /// 保存每次匿名空间声明。同一作用域中两次 namespace {} 是同一空间的重开，
  /// 有不同的源码范围，但 getCanonicalDecl() 相同，改名时必须共享新名字。
  std::vector<clang::NamespaceDecl *> Anonymous;
};

/// 规则前提需要的局部事实；不是通用控制流分析或等价性证明。
struct BodyFacts {
  bool HasJump = false; ///< goto、间接 goto 或 label；不包含 break/continue。
  bool NestedLambda = false;
  bool Unevaluated =
      false; ///< sizeof/alignof、decltype、typeid 等保守拒绝信号。
  std::vector<clang::DeclRefExpr *> References;
};

/// @param AST 完成解析的当前翻译单元。
/// @return 主文件清单；跳过头文件声明，仍保留宏展开到主文件的声明。
MainFileInventory collectMainFileInventory(clang::ASTContext &AST);

/// @param Body 要递归检查的语句树，通常为循环体或 lambda 的 body。
/// @return 该子树的跳转、嵌套闭包、未求值上下文及声明引用事实。
BodyFacts inspectBody(clang::Stmt *Body);

/// 单个翻译单元共享的借用上下文：连接只读 AST、配置和可写计划。
/// 这里集中源码位置操作；具体改写决策只放在 src/rules/*Rule.cpp 中。
/// 生命周期要求：AST、Plan、Options、Inventory 都必须比本对象活得久。
class RewriteContext {
public:
  clang::ASTContext &AST;
  /// SourceManager 将 SourceLocation 关联到文件、偏移及宏展开位置；借用自 AST。
  clang::SourceManager &SM;
  const clang::LangOptions &Lang;
  RewritePlan &Plan; ///< 规则只登记文本修改；后续 finish() 检查并统一应用。
  const FrontendOptions &Options;
  MainFileInventory &Inventory;

  /// @param AST 当前已解析 AST；其他引用由调用者持有，不转移所有权。
  /// @param Plan 供规则累积修改和拒绝原因的计划。
  /// @param Options 本次执行的只读配置。
  /// @param Inventory 已完成第一遍扫描的清单，freshName 会保留新增名字。
  RewriteContext(clang::ASTContext &AST, RewritePlan &Plan,
                 const FrontendOptions &Options, MainFileInventory &Inventory);

  /// @param L 源码位置，允许是宏位置。
  /// @return L 的展开位置是否位于主文件；这不代表宏文本可以直接修改。
  bool isMainFile(clang::SourceLocation L) const;
  /// @param L 一个原文 token 的起点。
  /// @return 该 token 后的半开区间终点；无效位置或宏位置返回无效位置。
  clang::SourceLocation tokenEnd(clang::SourceLocation L) const;
  /// @param R 包含首尾 token 的 Clang 范围。
  /// @return 保留原始拼写的文本副本；位置合法性由计划层最终检查。
  std::string sourceText(clang::SourceRange R) const;
  /// @param S 需要整体包块的完整语句。
  /// @return 语句最后 token 后的位置，并在紧跟分号时包含该分号。
  clang::SourceLocation statementEnd(const clang::Stmt *S) const;
  /// @param L 原文语句的起点。
  /// @return 该行开头到 L 的纯缩进；前缀包含其他代码时返回空串。
  std::string indentation(clang::SourceLocation L) const;
  /// @param Text 即将向内移动一级的原文片段。
  /// @return 第一行不变，其余非空行加两个空格后的副本。
  static std::string indentFollowingLines(llvm::StringRef Text);
  /// @param DC 声明所属上下文。
  /// @return 最近匿名命名空间的 canonical 声明；不存在则返回 nullptr。
  const clang::NamespaceDecl *
  anonymousOwner(const clang::DeclContext *DC) const;
  /// @param L 已换算为文件位置的原文位置，不能直接传宏位置。
  /// @param Owner 某匿名空间的 canonical 声明。
  /// @return L 是否位于该空间任一次重开的原文范围内。
  bool insideNamespace(clang::SourceLocation L,
                       const clang::NamespaceDecl *Owner) const;
  /// @param Base 建议的新名字。
  /// @return 不与主文件已知名字冲突的名字，并立即在 Inventory 中保留。
  std::string freshName(std::string Base);
  /// 保守检查取地址改写的已知风险，不声称完成任意模板/ADL 解析。
  /// @param D 将被取地址的类；同时检查定义、基类、友元和包围空间。
  /// @return 检测到一元 operator& 或定义未知时为 true，供捕获规则拒绝。
  bool hasAddressOperator(const clang::CXXRecordDecl *D) const;
};
} // namespace buddy::framac
#endif
