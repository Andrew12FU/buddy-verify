// SPDX-License-Identifier: Apache-2.0
#include "rules/GuardRule.h"
#include "clang/AST/ParentMapContext.h"

using namespace clang;
namespace buddy::framac {
void planGuardRewrite(IfStmt *S, RewriteContext &R) {
  if (!R.Options.enabled("guard") || !R.isMainFile(S->getBeginLoc()) ||
      S->getElse() || S->getInit() || S->getConditionVariable())
    return;
  // 内建 || 对应 BinaryOperator；重载 || 是另一种节点，不可混用。
  // 只包住整个 if，保留原条件、短路求值、返回和外部变量生命周期。
  auto *Or = dyn_cast<BinaryOperator>(S->getCond()->IgnoreParenImpCasts());
  if (!Or || Or->getOpcode() != BO_LOr ||
      !(isa<ReturnStmt>(S->getThen()) ||
        (R.Options.Profile == "silu-fusion" &&
         isa<CompoundStmt>(S->getThen()) &&
         cast<CompoundStmt>(S->getThen())->size() == 1 &&
         isa<ReturnStmt>(*cast<CompoundStmt>(S->getThen())->body_begin()))))
    return;
  auto Parents = R.AST.getParents(*S);
  if (Parents.size() != 1 || !Parents[0].get<CompoundStmt>()) {
    // 非块直接子语句不在当前范围内，跳过而不是猜测包块位置。
    return;
  }
  const auto *Block = Parents[0].get<CompoundStmt>();
  if (R.Options.Profile == "matmul-scalar" ||
      R.Options.Profile == "silu-fusion") {
    // Matmul / SiLU 只接受各自已审计方法体的直接子语句；循环内部或改写后内层块
    // 的 if 都不满足此父子关系。这既限制匹配范围，也保证重复执行不再包块。
    auto Owners = R.AST.getParents(*Block);
    const auto *Method =
        Owners.size() == 1 ? Owners[0].get<CXXMethodDecl>() : nullptr;
    if (!Method || Method->getBody() != Block ||
        Method->getNameAsString() != "matchAndRewrite" ||
        Method->getParent()->getNameAsString() !=
            (R.Options.Profile == "matmul-scalar" ? "ReplaceScalarLikeMatmul"
                                                  : "SiLUFusionPattern"))
      return;
  } else if (R.Options.Profile == "reshape") {
    // Reshape 仍采用窄匹配；新增 Matmul 不能扩大旧规则范围。
    // 否定可为普通 UnaryOperator，也可为真实 MLIR 的 operator! 调用。
    for (const Expr *Operand : {Or->getLHS(), Or->getRHS()}) {
      const Expr *Value = Operand->IgnoreParenImpCasts();
      if (const auto *Not = dyn_cast<UnaryOperator>(Value)) {
        if (Not->getOpcode() != UO_LNot)
          return;
        Value = Not->getSubExpr()->IgnoreParenImpCasts();
      } else if (const auto *Not = dyn_cast<CXXOperatorCallExpr>(Value)) {
        if (Not->getOperator() != OO_Exclaim || Not->getNumArgs() != 1)
          return;
        Value = Not->getArg(0)->IgnoreParenImpCasts();
      } else {
        return;
      }
      if (auto *Call = dyn_cast<CXXMemberCallExpr>(Value))
        Value = Call->getImplicitObjectArgument()->IgnoreParenImpCasts();
      auto *Ref = dyn_cast<DeclRefExpr>(Value);
      auto *Record = Ref ? Ref->getType()->getAsCXXRecordDecl() : nullptr;
      if (!Record ||
          Record->getQualifiedNameAsString() != "mlir::RankedTensorType")
        return;
    }
  }
  if (R.Options.Profile != "reshape" && R.Options.Profile != "matmul-scalar" &&
      R.Options.Profile != "silu-fusion" && R.Options.Profile != "eliminate-copy")
    return;
  const Stmt *Then = S->getThen();
  if (const auto *Body = dyn_cast<CompoundStmt>(Then))
    Then = *Body->body_begin();
  auto *Return = cast<ReturnStmt>(Then);
  const Expr *Value = Return->getRetValue();
  auto *Call =
      Value ? dyn_cast<CallExpr>(Value->IgnoreParenImpCasts()) : nullptr;
  if (!Call || !Call->getDirectCallee())
    return;
  auto Callee = Call->getDirectCallee()->getQualifiedNameAsString();
  if (Callee != "mlir::failure" && Callee != "llvm::failure")
    return;
  if (Block->size() == 1)
    return; // 已在独立块中；Reshape 不再重复包块。
  auto I = R.indentation(S->getBeginLoc());
  auto E = R.statementEnd(S);
  auto Original = R.Plan.Original.substr(
      R.SM.getFileOffset(S->getBeginLoc()),
      R.SM.getFileOffset(E) - R.SM.getFileOffset(S->getBeginLoc()));
  if (R.Options.Profile == "matmul-scalar" || R.Options.Profile == "eliminate-copy") {
    // A block containing only an if with scalar comparisons is flattened in
    // Frama-Clang's parseable output. Its short-circuit goto then shares the
    // enclosing block with destructor-marked objects declared without C
    // initializers, which Frama-C's destructor expansion rejects.
    // Keep a real block-local boolean: evaluate the original built-in || once,
    // in the same order, ending its full-expression before the failure return.
    // Do not move the tensor handles/ranks or change their lifetimes.
    const auto Name = R.freshName("guardFailed");
    const auto Condition = R.sourceText(S->getCond()->getSourceRange());
    const auto ReturnText = R.sourceText(Return->getSourceRange());
    R.Plan.add(R.SM, S->getBeginLoc(), E, "guard",
               "{\n" + I + "  bool " + Name + " = (" +
                   R.indentFollowingLines(Condition) + ");\n" + I +
                   "  if (" + Name + ")\n" + I + "    " + ReturnText +
                   ";\n" + I + "}",
               "Bind the original short-circuit condition in a block-local "
               "boolean so the conversion retains the guard scope",
               "One fresh built-in bool; no new external interface dependency");
    return;
  }
  R.Plan.add(
      R.SM, S->getBeginLoc(), E, "guard",
      "{\n" + I + "  " + R.indentFollowingLines(Original) + "\n" + I + "}",
      (R.Options.Profile == "matmul-scalar" ||
       R.Options.Profile == "silu-fusion")
          ? "Isolate a direct failure guard in the audited method "
            "pattern; keep loop guards unchanged"
          : "Isolate the complete known class-valued short-circuit guard");
  return;
}
} // namespace buddy::framac
