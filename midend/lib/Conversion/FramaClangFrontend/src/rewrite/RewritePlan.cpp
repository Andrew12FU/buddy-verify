// SPDX-License-Identifier: Apache-2.0
#include "rewrite/RewritePlan.h"
#include "clang/Basic/SourceManager.h"
#include <algorithm>
#include <tuple>

using namespace clang;
namespace buddy::framac {
void RewritePlan::reject(llvm::StringRef Message) {
  Rejections.push_back(Message.str());
}

bool RewritePlan::add(SourceManager &SM, SourceLocation Begin,
                      SourceLocation End, llvm::StringRef Rule,
                      std::string After, llvm::StringRef Reason,
                      llvm::StringRef DependencyChange) {
  if (Begin.isInvalid() || End.isInvalid() || Begin.isMacroID() ||
      End.isMacroID() || !SM.isWrittenInMainFile(Begin) ||
      !SM.isWrittenInMainFile(End)) {
    reject(("Uneditable/macro source range for " + Rule).str());
    return false;
  }
  unsigned Start = SM.getFileOffset(Begin), Finish = SM.getFileOffset(End);
  if (Finish < Start || Finish > Original.size()) {
    reject("Invalid source offsets");
    return false;
  }
  Edit E{Start,
         Finish - Start,
         SM.getSpellingLineNumber(Begin),
         SM.getSpellingColumnNumber(Begin),
         Rule.str(),
         Original.substr(Start, Finish - Start),
         std::move(After),
         Reason.str(),
         DependencyChange.str()};
  // 多个 AST 节点可能对应同一原文位置。相同结果去重，不同结果留待冲突检查。
  for (const auto &Old : Edits)
    if (Old.Offset == E.Offset && Old.Length == E.Length &&
        Old.After == E.After)
      return true;
  Edits.push_back(std::move(E));
  return true;
}

bool RewritePlan::addInsertion(SourceManager &SM, SourceLocation Location,
                               llvm::StringRef Rule, std::string Text,
                               int Order, llvm::StringRef Reason,
                               llvm::StringRef DependencyChange) {
  if (Order < 0) {
    reject("Invalid insertion order");
    return false;
  }
  size_t Count = Edits.size();
  if (!add(SM, Location, Location, Rule, std::move(Text), Reason,
           DependencyChange))
    return false;
  if (Edits.size() != Count)
    Edits.back().InsertionOrder = Order;
  return true;
}

// 先检查整份计划；出现任一拒绝时，不能输出局部成功的改写。
bool RewritePlan::finish() {
  std::sort(Edits.begin(), Edits.end(), [](const Edit &A, const Edit &B) {
    return std::tie(A.Offset, A.Length, A.InsertionOrder) <
           std::tie(B.Offset, B.Length, B.InsertionOrder);
  });
  for (size_t I = 1; I < Edits.size(); ++I) {
    const auto &A = Edits[I - 1];
    const auto &B = Edits[I];
    if (A.Offset == B.Offset && A.Length == 0 && B.Length == 0 &&
        A.InsertionOrder >= 0 && B.InsertionOrder > A.InsertionOrder)
      continue; // 显式依赖顺序；后向应用后得到从小到大的声明顺序。
    if (A.Offset + A.Length > B.Offset || A.Offset == B.Offset)
      reject("Overlapping edits: rerun rules in separate rounds");
  }
  if (!Parsed || !Rejections.empty())
    return false;
  // 后向应用使未处理部分的原始字节偏移不失效；不在上次产物上增量猜测。
  Rewritten = Original;
  for (auto I = Edits.rbegin(); I != Edits.rend(); ++I)
    Rewritten.replace(I->Offset, I->Length, I->After);
  return true;
}
} // namespace buddy::framac
