// SPDX-License-Identifier: Apache-2.0
#include "rules/ValueRangeRule.h"
#include "clang/AST/ParentMapContext.h"

using namespace clang;
namespace buddy::framac {
namespace {
/// @param E 带隐式转换的表达式；只读检查，不移除源码中的显式求值。
/// @param R 当前源码上下文。
/// @return 源码恰好只有一个自动局部对象引用时返回该引用，否则 nullptr。
/// 隐式 MLIR handle 转换仍可能有库语义；这里只限定已审计的调用输入形状。
const DeclRefExpr *localObject(Expr *E, RewriteContext &R) {
  auto Facts = inspectBody(E);
  if (Facts.References.size() != 1 || E->getBeginLoc().isMacroID() ||
      E->getEndLoc().isMacroID())
    return nullptr;
  const auto *Ref = Facts.References.front();
  const auto *V = dyn_cast<VarDecl>(Ref->getDecl());
  if (!V || (!V->hasLocalStorage() && !isa<ParmVarDecl>(V)) ||
      V->getType().isVolatileQualified() ||
      R.sourceText(E->getSourceRange()) != R.sourceText(Ref->getSourceRange()))
    return nullptr;
  return Ref;
}

/// @param S ValueRange 构造的子树。
/// @return 唯一数组列表；多个列表或没有列表时返回 nullptr，避免猜测嵌套构造。
InitListExpr *valueList(Stmt *S) {
  if (auto *List = dyn_cast<InitListExpr>(S))
    return List;
  InitListExpr *Found = nullptr;
  for (auto *Child : S->children()) {
    if (!Child)
      continue;
    if (auto *List = valueList(Child)) {
      if (Found)
        return nullptr;
      Found = List;
    }
  }
  return Found;
}

/// @param E 回调实参，允许隐式的 function_ref 构造和临时对象包装。
/// @return 包装中唯一的 lambda；任意调用或有显式捕获初始化时返回 nullptr。
LambdaExpr *directCallback(Expr *E) {
  if (auto *L = dyn_cast<LambdaExpr>(E)) {
    for (const auto &C : L->captures())
      if (C.capturesVariable() && C.getCapturedVar()->isInitCapture())
        return nullptr;
    return L;
  }
  Expr *Child = nullptr;
  if (!isa<ImplicitCastExpr>(E) && !isa<MaterializeTemporaryExpr>(E) &&
      !isa<CXXBindTemporaryExpr>(E) && !isa<ExprWithCleanups>(E) &&
      !isa<CXXConstructExpr>(E))
    return nullptr;
  for (auto *S : E->children()) {
    if (!S || isa<CXXDefaultArgExpr>(S))
      continue;
    if (Child || !isa<Expr>(S))
      return nullptr;
    Child = cast<Expr>(S);
  }
  return Child ? directCallback(Child) : nullptr;
}
} // namespace

void planValueRangeRewrite(CallExpr *Call, RewriteContext &R) {
  if (!R.Options.enabled("value-range") || R.Options.Profile != "silu-fusion" ||
      !R.isMainFile(Call->getBeginLoc()) || !Call->getDirectCallee() ||
      Call->getDirectCallee()->getQualifiedNameAsString() !=
          "mlir::linalg::GenericOp::create" ||
      Call->getNumArgs() < 8)
    return;
  InitListExpr *Lists[2]{};
  for (unsigned N = 0; N < 2; ++N) {
    auto *E = Call->getArg(N + 3);
    auto *Type = E->getType()->getAsCXXRecordDecl();
    if (Type && Type->getQualifiedNameAsString() == "mlir::ValueRange")
      Lists[N] = valueList(E);
  }
  if (!Lists[0] && !Lists[1])
    return; // 第二轮已经是具名 ValueRange，不再生成数组。
  if (!Lists[0] || !Lists[1]) {
    R.Plan.reject("value-range: both audited arguments must be lists");
    return;
  }
  // 新数组必须插在原声明之前，不能跨出分支，或把嵌套调用的实参提前求值。
  const Stmt *Node = Call;
  const VarDecl *V = nullptr;
  while (true) {
    auto Parents = R.AST.getParents(*Node);
    if (Parents.size() != 1)
      break;
    if ((V = Parents[0].get<VarDecl>()))
      break;
    const auto *E = Parents[0].get<Expr>();
    if (!E || isa<CallExpr>(E) || isa<BinaryOperator>(E) ||
        isa<ConditionalOperator>(E))
      break;
    Node = E;
  }
  const DeclStmt *Declaration = nullptr;
  if (V && V->hasLocalStorage()) {
    auto Parents = R.AST.getParents(*V);
    if (Parents.size() == 1)
      Declaration = Parents[0].get<DeclStmt>();
  }
  if (!Declaration || !Declaration->isSingleDecl() ||
      Declaration->getBeginLoc().isMacroID()) {
    R.Plan.reject("value-range: requires a single automatic declaration");
    return;
  }
  auto Parents = R.AST.getParents(*Declaration);
  if (Parents.size() != 1 || !Parents[0].get<CompoundStmt>()) {
    R.Plan.reject("value-range: declaration must be a direct block child");
    return;
  }
  for (unsigned N : {0u, 1u, 2u, 5u, 6u}) {
    if (!localObject(Call->getArg(N), R)) {
      R.Plan.reject("value-range: other arguments require evaluation review");
      return;
    }
  }
  if (!directCallback(Call->getArg(7))) {
    R.Plan.reject(
        "value-range: requires a direct callback without init-captures");
    return;
  }
  for (unsigned N = 8; N < Call->getNumArgs(); ++N) {
    if (!isa<CXXDefaultArgExpr>(Call->getArg(N))) {
      R.Plan.reject("value-range: additional explicit arguments need review");
      return;
    }
  }
  std::string Preamble;
  auto I = R.indentation(Declaration->getBeginLoc());
  for (unsigned N = 0; N < 2; ++N) {
    auto *List = Lists[N];
    if (!List->getNumInits()) {
      R.Plan.reject("value-range: empty arrays are unsupported");
      return;
    }
    std::string Values;
    for (auto *Element : List->inits()) {
      auto *Ref = localObject(Element, R);
      auto *Type = Ref ? Ref->getType()->getAsCXXRecordDecl() : nullptr;
      if (!Type ||
          (Type->getQualifiedNameAsString() != "mlir::Value" &&
           Type->getQualifiedNameAsString() != "mlir::tensor::EmptyOp")) {
        R.Plan.reject("value-range: only audited local Value/EmptyOp elements");
        return;
      }
      if (!Values.empty())
        Values += ", ";
      Values += R.sourceText(Element->getSourceRange());
    }
    auto Base = std::string(N == 0 ? "siluInput" : "siluOutput");
    auto Array = R.freshName(Base + "Values");
    auto Ref = R.freshName(Base + "Ref");
    auto Range = R.freshName(Base + "Range");
    Preamble += "mlir::Value " + Array + "[] = {" + Values + "};\n" + I +
                "llvm::ArrayRef<mlir::Value> " + Ref + "(" + Array +
                ", sizeof(" + Array + ") / sizeof(" + Array + "[0]));\n" + I +
                "mlir::ValueRange " + Range + "(" + Ref + ");\n" + I;
    auto *E = Call->getArg(N + 3);
    R.Plan.add(R.SM, E->getBeginLoc(), R.tokenEnd(E->getEndLoc()),
               "value-range", Range,
               "Use an array-backed range for the audited GenericOp argument",
               "Selects ValueRange(ArrayRef<Value>), not initializer_list");
  }
  R.Plan.addInsertion(
      R.SM, Declaration->getBeginLoc(), "value-range-preamble", Preamble, 10,
      "Own every Value element until the original call returns",
      "ArrayRef pointer/length and ValueRange(ArrayRef) required");
}
} // namespace buddy::framac
