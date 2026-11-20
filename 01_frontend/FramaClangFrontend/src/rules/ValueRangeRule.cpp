// SPDX-License-Identifier: Apache-2.0
#include "rules/ValueRangeRule.h"
#include "clang/AST/ParentMapContext.h"
#include <algorithm>
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;
namespace buddy::framac {
namespace {
class ArgumentConstruction : public RecursiveASTVisitor<ArgumentConstruction> {
public:
  bool Safe = true;
  bool TraverseLambdaExpr(LambdaExpr *) { return true; }
  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    auto *Ctor = E->getConstructor();
    const std::set<std::string> Views{
        "llvm::ArrayRef", "llvm::StringRef", "llvm::function_ref",
        "mlir::ValueRange", "mlir::TypeRange"};
    if (!Ctor->isTrivial() &&
        !Views.count(Ctor->getParent()->getQualifiedNameAsString()) &&
        !Ctor->getParent()->isLambda())
      Safe = false;
    return true;
  }
};

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
LambdaExpr *directCallback(Expr *E, RewriteContext &R) {
  if (auto *L = dyn_cast<LambdaExpr>(E)) {
    for (const auto &C : L->captures()) {
      if (C.getCaptureKind() == LCK_StarThis)
        return nullptr;
      if (C.capturesVariable() &&
          (C.getCapturedVar()->isInitCapture() ||
           (C.getCaptureKind() == LCK_ByCopy &&
            !C.getCapturedVar()->getType().isTriviallyCopyableType(R.AST))))
        return nullptr;
    }
    return L;
  }
  if (auto *Construct = dyn_cast<CXXConstructExpr>(E)) {
    auto *Owner = Construct->getType()->getAsCXXRecordDecl();
    if (!Owner || (!Owner->isLambda() &&
                   Owner->getQualifiedNameAsString() != "llvm::function_ref"))
      return nullptr;
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
  return Child ? directCallback(Child, R) : nullptr;
}
} // namespace

void planValueRangeRewrite(CallExpr *Call, RewriteContext &R) {
  if (!R.Options.enabled("value-range") || !R.isMainFile(Call->getBeginLoc()) ||
      !Call->getDirectCallee())
    return;
  struct Argument { unsigned Index; InitListExpr *List; };
  std::vector<Argument> Arguments;
  for (unsigned N = 0; N < Call->getNumArgs(); ++N) {
    auto *E = Call->getArg(N);
    if (isa<CXXDefaultArgExpr>(E))
      continue;
    auto *Type = E->getType()->getAsCXXRecordDecl();
    if (Type && Type->getQualifiedNameAsString() == "mlir::ValueRange")
      if (auto *List = valueList(E))
        Arguments.push_back({N, List});
  }
  if (Arguments.empty())
    return;
  // Find the whole expression or single automatic declaration in this block.
  // Never lift a list from another call, a conditional operand or a return.
  const Stmt *Anchor = Call;
  while (true) {
    auto Parents = R.AST.getParents(*Anchor);
    if (Parents.size() != 1) {
      R.Plan.reject("value-range: ambiguous evaluation owner");
      return;
    }
    if (Parents[0].get<CompoundStmt>())
      break;
    if (auto *V = Parents[0].get<VarDecl>()) {
      auto Owners = R.AST.getParents(*V);
      auto *D = Owners.size() == 1 ? Owners[0].get<DeclStmt>() : nullptr;
      if (!V->hasLocalStorage() || !D || !D->isSingleDecl()) {
        R.Plan.reject("value-range: requires one automatic declaration");
        return;
      }
      auto Blocks = R.AST.getParents(*D);
      if (Blocks.size() != 1 || !Blocks[0].get<CompoundStmt>()) {
        R.Plan.reject("value-range: requires one automatic declaration directly in a block");
        return;
      }
      Anchor = D;
      break;
    }
    auto *E = Parents[0].get<Expr>();
    if (!E || !isa<ImplicitCastExpr, MaterializeTemporaryExpr,
                   CXXBindTemporaryExpr, ExprWithCleanups>(E)) {
      R.Plan.reject("value-range: nested/conditional evaluation needs a local block");
      return;
    }
    Anchor = E;
  }
  if (auto *Member = dyn_cast<CXXMemberCallExpr>(Call)) {
    auto *Object = Member->getImplicitObjectArgument();
    if (!localObject(Object, R) && !isa<CXXThisExpr>(Object->IgnoreParenImpCasts())) {
      R.Plan.reject("value-range: member object evaluation requires review");
      return;
    }
  }
  // Other arguments must not acquire or move observable evaluation effects.
  for (unsigned N = 0; N < Call->getNumArgs(); ++N) {
    if (std::any_of(Arguments.begin(), Arguments.end(),
                    [N](const Argument &A) { return A.Index == N; }))
      continue;
    auto *E = Call->getArg(N);
    if (isa<CXXDefaultArgExpr>(E))
      continue;
    if (localObject(E, R)) {
      ArgumentConstruction Construction;
      Construction.TraverseStmt(E);
      if (Construction.Safe)
        continue;
      R.Plan.reject("value-range: implicit argument construction has unknown effects");
      return;
    }
    auto *Simple = E->IgnoreParenImpCasts();
    if (isa<IntegerLiteral, FloatingLiteral, CharacterLiteral, StringLiteral,
            CXXBoolLiteralExpr, CXXNullPtrLiteralExpr>(Simple) || directCallback(E, R))
      continue;
    R.Plan.reject("value-range: other arguments require evaluation review");
    return;
  }
  std::string Preamble;
  auto I = R.indentation(Anchor->getBeginLoc());
  for (const auto &Argument : Arguments) {
    auto *List = Argument.List;
    if (!List->getNumInits()) {
      R.Plan.reject("value-range: empty arrays are unsupported");
      return;
    }
    std::string Values;
    for (auto *Element : List->inits()) {
      auto *Ref = localObject(Element, R);
      auto *Type = Ref ? Ref->getType()->getAsCXXRecordDecl() : nullptr;
      // These MLIR handles have the reviewed Value conversion contract. An
      // arbitrary user conversion could mutate state when lifted before a call.
      if (!Type || (Type->getQualifiedNameAsString() != "mlir::Value" &&
                    Type->getQualifiedNameAsString() != "mlir::tensor::EmptyOp")) {
        R.Plan.reject("value-range: only audited local Value/EmptyOp elements");
        return;
      }
      if (!Values.empty())
        Values += ", ";
      Values += R.sourceText(Element->getSourceRange());
    }
    auto Array = R.freshName("rangeValues");
    auto Ref = R.freshName("rangeRef");
    auto Range = R.freshName("valueRange");
    Preamble += "mlir::Value " + Array + "[] = {" + Values + "};\n" + I +
                "llvm::ArrayRef<mlir::Value> " + Ref + "(" + Array +
                ", sizeof(" + Array + ") / sizeof(" + Array + "[0]));\n" + I +
                "mlir::ValueRange " + Range + "(" + Ref + ");\n" + I;
    auto *E = Call->getArg(Argument.Index);
    R.Plan.add(R.SM, E->getBeginLoc(), R.tokenEnd(E->getEndLoc()),
               "value-range", Range,
               "Use an array-backed ValueRange argument identified by AST type",
               "Selects ValueRange(ArrayRef<Value>), not initializer_list");
  }
  R.Plan.addInsertion(R.SM, Anchor->getBeginLoc(), "value-range-preamble",
                      Preamble, 10,
                      "Own Value elements in the original call block",
                      "No fixed callee, argument position/count, callback slot or profile");
}
} // namespace buddy::framac
