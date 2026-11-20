// SPDX-License-Identifier: Apache-2.0
#include "rules/WhileRule.h"
#include "clang/AST/ParentMapContext.h"
#include <algorithm>

using namespace clang;
namespace buddy::framac {
// W 是 clang::WhileStmt *，借用整条 while 的 AST 节点；R 是共享上下文的引用，
// 通过它读取源码、配置并写入 Plan。函数返回 void，不直接修改 AST 或写输出文件。
void planWhileRewrite(WhileStmt *W, RewriteContext &R) {
  if (!R.Options.enabled("while") || !R.isMainFile(W->getBeginLoc()) ||
      !W->getConditionVariable())
    return;
  // 普通 while 不匹配；条件声明必须每轮执行，不能提到循环外初始化一次。
  // V 推导为 clang::VarDecl *。例如 while (auto inner =
  // base.getDefiningOp<...>()) 中，W 对应整个循环，V 只对应 auto inner = ...
  // 这条条件变量声明。
  auto *V = W->getConditionVariable();
  auto Facts = inspectBody(W->getBody());
  if (Facts.HasJump || W->getBeginLoc().isMacroID() ||
      !isa<CompoundStmt>(W->getBody()) || V->getType()->isReferenceType() ||
      V->getType().isVolatileQualified() ||
      V->getType().isDestructedType() != QualType::DK_none ||
      isa<ExprWithCleanups>(V->getInit())) {
    R.Plan.reject("while: unsupported jump/body/lifetime structure");
    return;
  }
  // 保留原 body 为内层块，使其中同名局部变量仍可遮蔽条件变量。
  // continue 回到下一轮初始化；先显式转 bool 再取反，避免选中重载 !。
  // 例如 while (int n = next()) { use(n); } 生成：
  //   while (true) {
  //     int n = next();
  //     if (!static_cast<bool>(n)) break;
  //     { use(n); }
  //   }
  // sourceText(V->getSourceRange())
  // 保留原声明拼写，再补分号；循环体也从原文读取。
  auto I = R.indentation(W->getBeginLoc());
  auto New =
      "while (true) {\n" + I + "  " + R.sourceText(V->getSourceRange()) +
      ";\n" + I + "  if (!static_cast<bool>(" + V->getNameAsString() + "))\n" +
      I + "    break;\n" + I + "  " +
      R.indentFollowingLines(R.sourceText(W->getBody()->getSourceRange())) +
      "\n" + I + "}";
  // 登记从 while 关键字到末尾 } 之后的文本替换。Plan.finish()
  // 后续检查整份计划， 无拒绝或范围冲突才应用到原文副本；这里 add
  // 并不立即执行改写。
  R.Plan.add(
      R.SM, W->getBeginLoc(), R.tokenEnd(W->getEndLoc()), "while", New,
      "Reevaluate each iteration; preserve body scope and contextual bool",
      "No new library API; explicit bool conversion");
  return;
}

} // namespace buddy::framac
