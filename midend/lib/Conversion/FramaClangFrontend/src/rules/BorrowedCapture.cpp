// SPDX-License-Identifier: Apache-2.0
#include "rules/BorrowedCapture.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <algorithm>
using namespace clang;
namespace buddy::framac {
namespace {
/// 只接受通过 this 读取的普通标量字段；方法调用、位域和复合 this 用法
/// 没有等价的字段地址，不能套用此规则。字段以声明身份分组。
class MemberUses : public RecursiveASTVisitor<MemberUses> {
  RewriteContext &R;

public:
  bool Valid = true;
  std::vector<MemberExpr *> Uses;
  explicit MemberUses(RewriteContext &R) : R(R) {}
  bool VisitCXXThisExpr(CXXThisExpr *E) {
    auto P = R.AST.getParents(*E);
    auto *M = P.size() == 1 ? P[0].get<MemberExpr>() : nullptr;
    auto *F = M ? dyn_cast<FieldDecl>(M->getMemberDecl()) : nullptr;
    if (!F || F->isBitField() || !F->getType()->isIntegerType() ||
        F->getType().isVolatileQualified())
      Valid = false;
    else
      Uses.push_back(const_cast<MemberExpr *>(M));
    return true;
  }
};
} // namespace
void planBorrowedCaptureRewrite(LambdaExpr *L, RewriteContext &R) {
  if (L->capture_size() == 0)
    return;
  auto Facts = inspectBody(L->getBody());
  if (L->isGenericLambda() || Facts.NestedLambda || Facts.Unevaluated ||
      L->getBeginLoc().isMacroID()) {
    R.Plan.reject(
        "borrowed-capture: generic/nested/macro/unevaluated callback");
    return;
  }
  // 指针仅放在原调用所在块中，避免跨条件、循环或声明改变求值路径。
  const Stmt *Anchor = L;
  while (true) {
    auto P = R.AST.getParents(*Anchor);
    if (P.size() != 1) {
      R.Plan.reject("borrowed-capture: ambiguous callback owner");
      return;
    }
    if (P[0].get<CompoundStmt>())
      break;
    auto *S = P[0].get<Stmt>();
    if (!S || !isa<Expr>(S)) {
      R.Plan.reject("borrowed-capture: requires a direct block expression");
      return;
    }
    Anchor = S;
  }
  std::map<const VarDecl *, std::string> Variables;
  std::map<const FieldDecl *, std::string> Fields;
  std::string Prelude, Captures;
  auto bind = [&](std::string Name, std::string Expression) {
    auto Pointer = R.freshName(Name + "Ptr");
    Prelude += "auto *" + Pointer + " = &(" + Expression + ");\n" +
               R.indentation(Anchor->getBeginLoc());
    if (!Captures.empty())
      Captures += ", ";
    Captures += Pointer;
    return Pointer;
  };
  bool HasThis = false, HasReference = false;
  for (const auto &C : L->captures()) {
    if (C.capturesThis()) {
      if (C.getCaptureKind() != LCK_This) {
        R.Plan.reject("borrowed-capture: copying *this is unsupported");
        return;
      }
      HasThis = true;
      continue;
    }
    auto *V =
        C.capturesVariable() ? dyn_cast<VarDecl>(C.getCapturedVar()) : nullptr;
    // On a second run every capture is already a pointer by value.
    if (!V || V->isInitCapture()) {
      R.Plan.reject("borrowed-capture: unsupported capture");
      return;
    }
    HasReference |= C.getCaptureKind() == LCK_ByRef;
    if (C.getCaptureKind() != LCK_ByRef)
      continue;
    auto Q = V->getType().getNonReferenceType();
    if (Q.isVolatileQualified() ||
        (Q->isRecordType() && R.hasAddressOperator(Q->getAsCXXRecordDecl()))) {
      R.Plan.reject("borrowed-capture: volatile/overloaded-address object");
      return;
    }
    Variables[V->getCanonicalDecl()] =
        bind(V->getNameAsString(), V->getNameAsString());
  }
  if (!HasThis && !HasReference)
    return;
  // A mixture with original value captures would require preserving each
  // original capture's construction. This bounded rule only takes references.
  for (const auto &C : L->captures())
    if (!C.capturesThis() && C.getCaptureKind() != LCK_ByRef) {
      R.Plan.reject("borrowed-capture: mixed value/reference capture");
      return;
    }
  MemberUses Members(R);
  Members.TraverseStmt(L->getBody());
  if (!Members.Valid) {
    R.Plan.reject(
        "borrowed-capture: this is not an ordinary scalar field access");
    return;
  }
  for (auto *M : Members.Uses) {
    auto *F = cast<FieldDecl>(M->getMemberDecl());
    if (!Fields.count(F))
      Fields[F] = bind(F->getNameAsString(), "this->" + F->getNameAsString());
  }
  struct Edit {
    unsigned Begin, End;
    std::string Text;
  };
  unsigned Begin = R.SM.getFileOffset(L->getBeginLoc());
  unsigned End = R.SM.getFileOffset(R.tokenEnd(L->getEndLoc()));
  std::vector<Edit> Edits;
  auto replace = [&](SourceRange Range, std::string Text) {
    if (Range.getBegin().isMacroID() || Range.getEnd().isMacroID()) {
      R.Plan.reject("borrowed-capture: macro use");
      return;
    }
    Edits.push_back({R.SM.getFileOffset(Range.getBegin()) - Begin,
                     R.SM.getFileOffset(R.tokenEnd(Range.getEnd())) - Begin,
                     Text});
  };
  replace(L->getIntroducerRange(), "[" + Captures + "]");
  for (auto *Ref : Facts.References)
    if (auto *V = dyn_cast<VarDecl>(Ref->getDecl()))
      if (Variables.count(V->getCanonicalDecl()))
        replace(Ref->getSourceRange(),
                "(*" + Variables[V->getCanonicalDecl()] + ")");
  for (auto *M : Members.Uses)
    replace(M->getSourceRange(),
            "(*" + Fields[cast<FieldDecl>(M->getMemberDecl())] + ")");
  std::sort(Edits.begin(), Edits.end(),
            [](const Edit &A, const Edit &B) { return A.Begin > B.Begin; });
  auto Text = R.Plan.Original.substr(Begin, End - Begin);
  unsigned Previous = End - Begin;
  for (const auto &E : Edits) {
    if (E.End > Previous) {
      R.Plan.reject("borrowed-capture: overlapping captured uses");
      return;
    }
    Text.replace(E.Begin, E.End - E.Begin, E.Text);
    Previous = E.Begin;
  }
  R.Plan.addInsertion(
      R.SM, Anchor->getBeginLoc(), "capture-pointers", Prelude, 20,
      "Borrow the original local, reference referent and per-object scalar "
      "fields",
      "Callback still depends on the lifetime of the original objects");
  R.Plan.add(R.SM, L->getBeginLoc(), R.tokenEnd(L->getEndLoc()), "capture",
             Text,
             "Capture borrowed pointers by value without copying rewriter or "
             "globalizing members");
}
} // namespace buddy::framac
