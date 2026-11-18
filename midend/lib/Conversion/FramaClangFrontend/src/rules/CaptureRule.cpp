// SPDX-License-Identifier: Apache-2.0
#include "rules/CaptureRule.h"
#include "rules/BorrowedCapture.h"
#include "rules/NestedCapture.h"
#include "clang/AST/ParentMapContext.h"
#include <algorithm>

using namespace clang;
namespace buddy::framac {
// 审查顺序：候选筛选 → 安全前提 → 插入位置 → 声明映射 → 局部文本替换
// → 全局计划。规则不改变 AST，也不证明闭包逃逸后的生命周期。
void planCaptureRewrite(LambdaExpr *L, RewriteContext &R) {
  if (!R.Options.enabled("capture") || !R.isMainFile(L->getBeginLoc()))
    return;
  if (R.Options.Profile == "builtin-transpose") {
    planBorrowedCaptureRewrite(L, R);
    return;
  }
  // 先让最外层 lambda 接管整棵嵌套树，避免父子分别提交重叠替换。
  // 非嵌套样例仍走以下原路径，保持旧 pass 的输出及匹配边界。
  if (planNestedCaptureRewrite(L, R))
    return;
  // 捕获一个类对象，与捕获已经是 T& 的引用变量，是不同的转换路径。
  bool NeedsRewrite = false;
  for (const auto &C : L->captures())
    NeedsRewrite |= C.capturesVariable() && C.getCaptureKind() == LCK_ByRef &&
                    !C.getCapturedVar()->getType()->isReferenceType() &&
                    C.getCapturedVar()->getType()->isRecordType();
  if (!NeedsRewrite)
    return;
  auto Facts = inspectBody(L->getBody());
  if (L->isGenericLambda() || Facts.NestedLambda || Facts.Unevaluated ||
      L->getBeginLoc().isMacroID()) {
    R.Plan.reject(
        "capture: generic/nested/macro/unevaluated lambda needs review");
    return;
  }
  // 接受同一块中的直接表达式语句，SiLU 另接受单个自动变量初始化。
  // 不能将新增声明提前到分支、返回、
  // 条件表达式或更外层循环，否则可能改变执行路径或对象可见性。
  const Stmt *Statement = L;
  while (true) {
    auto Parents = R.AST.getParents(*Statement);
    if (Parents.size() != 1) {
      R.Plan.reject("capture: ambiguous enclosing statement");
      return;
    }
    if (Parents[0].get<CompoundStmt>())
      break;
    if (const auto *V = Parents[0].get<VarDecl>()) {
      // SiLU 的回调位于单个自动局部变量的初始化表达式中。
      // 指针仍插在同一块的该声明之前，不提前跨过其他语句或分支。
      auto Owners = R.AST.getParents(*V);
      const auto *D = Owners.size() == 1 ? Owners[0].get<DeclStmt>() : nullptr;
      if ((R.Options.Profile != "silu-fusion" &&
           R.Options.Profile != "depthwise-conv") ||
          !V->hasLocalStorage() || !D || !D->isSingleDecl()) {
        R.Plan.reject("capture: unsupported declaration initializer");
        return;
      }
      auto Blocks = R.AST.getParents(*D);
      if (Blocks.size() != 1 || !Blocks[0].get<CompoundStmt>()) {
        R.Plan.reject("capture: declaration must be a direct block child");
        return;
      }
      Statement = D;
      break;
    }
    const auto *Parent = Parents[0].get<Stmt>();
    if (!Parent || isa<ReturnStmt>(Parent) || isa<DeclStmt>(Parent) ||
        isa<IfStmt>(Parent) || isa<BinaryOperator>(Parent) ||
        isa<ConditionalOperator>(Parent)) {
      R.Plan.reject(
          "capture: requires a direct expression statement in a block");
      return;
    }
    Statement = Parent;
  }
  // 以 canonical 声明身份为键，同名但不同作用域的局部对象不会被误改。
  std::map<const VarDecl *, std::string> Pointers;
  std::string Declarations, Captures;
  for (const auto &C : L->captures()) {
    auto *V =
        C.capturesVariable() ? dyn_cast<VarDecl>(C.getCapturedVar()) : nullptr;
    if (!V || C.getCaptureKind() != LCK_ByRef || V->isInitCapture() ||
        V->getType()->isReferenceType() || !V->getType()->isRecordType() ||
        R.hasAddressOperator(V->getType()->getAsCXXRecordDecl())) {
      R.Plan.reject("capture: mixed/this/reference-variable/overloaded-address "
                    "capture");
      return;
    }
    auto Name = R.freshName(V->getNameAsString() + "Ptr");
    Pointers[V->getCanonicalDecl()] = Name;
    if (!Captures.empty())
      Captures += ", ";
    Captures += Name;
    Declarations += "auto *" + Name + " = &" + V->getNameAsString() + ";\n" +
                    R.indentation(Statement->getBeginLoc());
  }
  // LocalEdit 的偏移相对 lambda 开头；外层 Plan 则相对整个原始主文件。
  // 先合并闭包内部编辑，避免全局计划出现相互嵌套的替换范围。
  unsigned Start = R.SM.getFileOffset(L->getBeginLoc());
  unsigned Finish = R.SM.getFileOffset(R.tokenEnd(L->getEndLoc()));
  struct LocalEdit {
    unsigned Offset, Length;
    std::string Text;
  };
  auto Intro = L->getIntroducerRange();
  std::vector<LocalEdit> Local{{R.SM.getFileOffset(Intro.getBegin()) - Start,
                                R.SM.getFileOffset(R.tokenEnd(Intro.getEnd())) -
                                    R.SM.getFileOffset(Intro.getBegin()),
                                "[" + Captures + "]"}};
  for (auto *Ref : Facts.References) {
    auto *V = dyn_cast<VarDecl>(Ref->getDecl());
    if (!V || !Pointers.count(V->getCanonicalDecl()))
      continue;
    if (Ref->getBeginLoc().isMacroID() || Ref->getEndLoc().isMacroID()) {
      R.Plan.reject("capture: a captured object is used through a macro");
      return;
    }
    unsigned B = R.SM.getFileOffset(Ref->getBeginLoc());
    unsigned E = R.SM.getFileOffset(R.tokenEnd(Ref->getEndLoc()));
    Local.push_back(
        {B - Start, E - B, "(*" + Pointers[V->getCanonicalDecl()] + ")"});
  }
  // 后向应用，前面尚未替换部分的原始偏移才保持有效。
  std::sort(Local.begin(), Local.end(),
            [](const auto &A, const auto &B) { return A.Offset > B.Offset; });
  auto New = R.Plan.Original.substr(Start, Finish - Start);
  for (const auto &E : Local)
    New.replace(E.Offset, E.Length, E.Text);
  // 两项全局编辑：插入原对象指针，再替换闭包。任何拒绝都会阻止整份输出。
  R.Plan.addInsertion(
      R.SM, Statement->getBeginLoc(), "capture-pointers", Declarations, 20,
      "Take the addresses of the original captured objects",
      "Pointer locals; callback lifetime assumptions remain unchanged");
  R.Plan.add(R.SM, L->getBeginLoc(), R.tokenEnd(L->getEndLoc()), "capture", New,
             "Capture pointers by value; rewrite references by declaration "
             "identity");
  return;
}

} // namespace buddy::framac
