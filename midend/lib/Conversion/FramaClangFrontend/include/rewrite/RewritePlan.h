// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_REWRITE_PLAN_H
#define BUDDY_FRAMAC_REWRITE_PLAN_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <vector>

namespace clang {
class SourceManager;
}
namespace buddy::framac {
/// 单个补丁。Offset/Length 均为原始 UTF-8 缓冲区的字节数，不能当字符数。
/// 替换范围为 [Offset, Offset + Length)，Length==0 表示插入。
struct Edit {
  unsigned Offset, Length;
  unsigned Line, Column; ///< 原文中的 1 基行列，只用于审查和报告。
  std::string Rule, Before, After;
  std::string Reason, DependencyChange; ///< 依据及后续接口提取需要关注的变化。
  int InsertionOrder = -1; ///< 非负仅用于显式协作的同位置插入；其他碰撞仍拒绝。
};

/// 收集规则结果，再统一检查冲突；不负责解析、文件写入或调用 Frama-C。
/// 所有规则始终读取 Original，直到 finish() 才生成 Rewritten。
/// AST 指针不存入计划，因此计划可在 AST 生命周期结束后序列化。
struct RewritePlan {
  std::string Input, Original, Rewritten;
  std::vector<Edit> Edits;
  std::vector<std::string> Rejections;
  bool Parsed = false; ///< 仅表示原始翻译单元已通过 Clang 解析。

  /// @param Message 本次整份计划不能接受的原因；只记录，不抛异常。
  void reject(llvm::StringRef Message);

  /// 添加一个候选修改，并拒绝宏展开、跨文件或无效位置。
  /// @param SM 本轮 Clang 的源码管理器，仅在调用期间借用。
  /// @param Begin 原始主文件中的起点。
  /// @param End 原始主文件中不包含的终点，调用者须完成 token 终点换算。
  /// @param Rule 可追溯的规则标识。
  /// @param After 替换文本，空串表示删除。
  /// @param Reason 匹配依据或保持条件，写入报告。
  /// @param DependencyChange 新的依赖或已知性质变化，默认 "none"。
  /// @return 修改被加入或与已有相同修改合并时为 true；拒绝时为 false。
  /// @note true 不是最终成功；后续 finish() 仍可能发现冲突。
  bool add(clang::SourceManager &SM, clang::SourceLocation Begin,
           clang::SourceLocation End, llvm::StringRef Rule, std::string After,
           llvm::StringRef Reason, llvm::StringRef DependencyChange = "none");

  /// 添加可协作的语句前声明；只接受零长度插入，按 Order 从小到大输出。
  /// @param SM/Location 原始主文件中的插入位置，宏位置仍拒绝。
  /// @param Rule/Text/Reason/DependencyChange 含义同 add。
  /// @param Order 明确的依赖顺序：ValueRange 数组为 10，捕获指针为 20。
  /// @return 登记成功为 true；负序号或同位置同序号冲突导致整份计划拒绝。
  bool addInsertion(clang::SourceManager &SM, clang::SourceLocation Location,
                    llvm::StringRef Rule, std::string Text, int Order,
                    llvm::StringRef Reason,
                    llvm::StringRef DependencyChange = "none");

  /// 排序、检测重叠，并从后向前应用补丁，避免前面的偏移失效。
  /// @return 解析通过且无拒绝原因时为 true，并填充 Rewritten；否则 false。
  /// @note 只生成内存文本；不会写文件，也不证明 C++/Frama-Clang 转换成功。
  bool finish();
};
} // namespace buddy::framac
#endif
