// SPDX-License-Identifier: Apache-2.0
#include "rules/InitializerListRule.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
using namespace clang;
namespace buddy::framac {
namespace {
// Only unwrap initialization nodes. Looking through another constructor or a
// builder call could mistake an inner shape/attribute list for this range's
// own element list and select a different constructor.
/// @param S 构造实参及其隐式初始化包装节点，借用 AST。
/// @return 该实参自身的列表；遇到其他调用/构造即停止，避免误取内层列表。
InitListExpr *directList(Stmt *S) {
  if (!S)
    return nullptr;
  if (auto *L = dyn_cast<InitListExpr>(S))
    return L;
  if (!isa<ExprWithCleanups, MaterializeTemporaryExpr, CXXBindTemporaryExpr,
           ImplicitCastExpr, CXXStdInitializerListExpr>(S))
    return nullptr;
  for (auto *Child : S->children())
    if (auto *L = directList(Child))
      return L;
  return nullptr;
}
InitListExpr *ownList(CXXConstructExpr *E) {
  for (auto *Arg : E->arguments())
    if (auto *L = directList(Arg))
      return L;
  return nullptr;
}
// Nested calls are rewritten from the leaves on fresh ASTs. An enclosing
// replacement must never retain stale text over a child replacement.
bool hasNestedRangeList(Stmt *S) {
  if (!S || isa<LambdaExpr>(S))
    return false;
  if (auto *E = dyn_cast<CXXConstructExpr>(S)) {
    auto *C = E->getType()->getAsCXXRecordDecl();
    auto Name = C ? C->getQualifiedNameAsString() : "";
    if ((Name == "mlir::ValueRange" || Name == "mlir::TypeRange" ||
         Name == "llvm::ArrayRef") &&
        ownList(E))
      return true;
  }
  for (auto *Child : S->children())
    if (hasNestedRangeList(Child))
      return true;
  return false;
}
// Semantic/syntactic InitListExpr forms may share child nodes. Inspect every
// parent path instead of treating that AST DAG as two evaluations. All paths
// must end at the same block statement and contain no conditional evaluation.
/// @param Start 候选构造；Anchor 为同一块中的完整表达式/单个声明语句。
/// @param R 提供 AST 父节点查询，不在本函数提交编辑。
/// @return 所有父路径均到达 Anchor 且无短路/条件/逗号求值时为 true。
bool unconditionalPaths(const Stmt *Start, const Stmt *Anchor,
                        RewriteContext &R) {
  std::vector<const Stmt *> Pending{Start};
  std::set<const Stmt *> Seen;
  bool Reached = false;
  while (!Pending.empty()) {
    auto *Node = Pending.back();
    Pending.pop_back();
    if (Node == Anchor) {
      Reached = true;
      continue;
    }
    if (!Seen.insert(Node).second)
      continue;
    auto Parents = R.AST.getParents(*Node);
    if (Parents.empty())
      return false;
    for (const auto &P : Parents) {
      if (auto *S = P.get<Stmt>()) {
        if (isa<ConditionalOperator, BinaryConditionalOperator>(S) ||
            (isa<BinaryOperator>(S) &&
             (cast<BinaryOperator>(S)->isLogicalOp() ||
              cast<BinaryOperator>(S)->getOpcode() == BO_Comma)))
          return false;
        Pending.push_back(S);
      } else if (auto *V = P.get<VarDecl>()) {
        auto Owners = R.AST.getParents(*V);
        if (Owners.size() != 1 || !Owners[0].get<DeclStmt>())
          return false;
        Pending.push_back(Owners[0].get<DeclStmt>());
      } else
        return false;
    }
  }
  return Reached;
}
InitListExpr *findList(Stmt *S) {
  if (!S)
    return nullptr;
  if (auto *L = dyn_cast<InitListExpr>(S))
    return L;
  if (isa<LambdaExpr>(S))
    return nullptr;
  for (auto *Child : S->children())
    if (auto *L = findList(Child))
      return L;
  return nullptr;
}
/// Inspect list elements without rewriting their expressions. Only the reviewed
/// handle query/conversion family is accepted; arbitrary side effects reject.
class ElementCalls : public RecursiveASTVisitor<ElementCalls> {
  bool Transpose;

public:
  explicit ElementCalls(bool Transpose = false) : Transpose(Transpose) {}
  bool Valid = true;
  std::string RejectedCall;
  bool VisitCallExpr(CallExpr *E) {
    auto *D = E->getDirectCallee();
    if (!D) {
      Valid = false;
      return true;
    }
    // A spelling alone is insufficient: a user's getResult/operator[] may have
    // effects. Limit hoisted queries to the actual MLIR/LLVM owner families.
    auto *M = dyn_cast<CXXMethodDecl>(D);
    auto Owner = M ? M->getParent()->getQualifiedNameAsString() : "";
    bool Result = D->getNameAsString() == "getResult" &&
                  Owner == "mlir::OpTrait::detail::MultiResultTraitBase";
    bool Index = D->getOverloadedOperator() == OO_Subscript &&
                 Owner == "llvm::detail::indexed_accessor_range_base";
    bool Conversion = isa<CXXConversionDecl>(D) &&
                      Owner == "mlir::OpTrait::OneTypedResult::Impl";
    if (Transpose && isa<CXXConversionDecl>(D) && M &&
        M->getParent()->getName() == "Impl")
      if (auto *Outer = dyn_cast<ClassTemplateSpecializationDecl>(
              M->getParent()->getDeclContext()))
        Conversion |=
            Outer->getSpecializedTemplate()->getQualifiedNameAsString() ==
            "mlir::OpTrait::OneTypedResult";
    // Immutable metadata factories in the audited transpose call sites. IR
    // creation, unknown free functions and similarly named user methods reject.
    // Hoisting is bounded to these calls in one original execution block.
    const std::set<std::string> BuilderQueries{
        "getNamedAttr",    "getI32TensorAttr", "getArrayAttr",
        "getI64ArrayAttr", "getBoolArrayAttr", "getDenseI32ArrayAttr",
        "getContext",      "getI1Type"};
    auto Op = D->getOverloadedOperator();
    bool AffineArithmetic =
        Op == OO_Plus || Op == OO_Minus || Op == OO_Star || Op == OO_Percent;
    bool Metadata =
        Transpose &&
        ((Owner == "mlir::Builder" &&
          BuilderQueries.count(D->getNameAsString())) ||
         (Owner == "mlir::AffineExpr" &&
          (D->getNameAsString() == "floorDiv" || AffineArithmetic)) ||
         ((Owner == "mlir::AffineMap" || Owner == "mlir::AffineMapAttr" ||
           Owner == "mlir::VectorType") &&
          D->getNameAsString() == "get"));
    if (!Result && !Index && !Conversion && !Metadata) {
      Valid = false;
      RejectedCall = Owner + "::" + D->getNameAsString();
    }
    return true;
  }
  bool VisitBinaryOperator(BinaryOperator *E) {
    if (E->isAssignmentOp())
      Valid = false;
    return true;
  }
  bool VisitUnaryOperator(UnaryOperator *E) {
    if (E->isIncrementDecrementOp())
      Valid = false;
    return true;
  }
};
class Lists : public RecursiveASTVisitor<Lists> {
  RewriteContext &R;
  Stmt *Anchor;
  std::string Preamble;
  std::set<const CXXConstructExpr *> Visited;

public:
  Lists(RewriteContext &R, Stmt *Anchor) : R(R), Anchor(Anchor) {}
  bool TraverseLambdaExpr(LambdaExpr *) { return true; }
  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    if (!Visited.insert(E).second)
      return true;
    auto *C = E->getType()->getAsCXXRecordDecl();
    if (!C)
      return true;
    auto Name = C->getQualifiedNameAsString();
    bool Transpose = R.Options.Profile == "builtin-transpose";
    if (Name != "mlir::ValueRange" &&
        !(Transpose && Name == "mlir::TypeRange") &&
        Name != "llvm::SmallVector" && Name != "llvm::ArrayRef")
      return true;
    // Only a written brace construction. Constructors around an existing named
    // range or pointer/length pair have no initializer list and are idempotent.
    auto *L = Transpose ? ownList(E) : findList(E);
    if (!L || !L->getNumInits())
      return true;
    if (Transpose)
      for (auto *Value : L->inits())
        if (hasNestedRangeList(Value))
          return true;
    // Hoisting out of a conditional operand would execute a previously skipped
    // query. Stop at nested callbacks and reject
    // conditional/comma/short-circuit contexts instead of silently changing
    // their evaluation path.
    const Stmt *Node = E;
    if (Transpose && !unconditionalPaths(E, Anchor, R)) {
      R.Plan.reject("init-list: all nested-list parent paths must be "
                    "unconditional in the original block statement");
      return true;
    }
    while (!Transpose && Node != Anchor) {
      auto Parents = R.AST.getParents(*Node);
      if (Parents.size() != 1) {
        R.Plan.reject("init-list: ambiguous evaluation context");
        return true;
      }
      if (auto *Parent = Parents[0].get<Stmt>()) {
        if (isa<ConditionalOperator, BinaryConditionalOperator>(Parent) ||
            (isa<BinaryOperator>(Parent) &&
             (cast<BinaryOperator>(Parent)->isLogicalOp() ||
              cast<BinaryOperator>(Parent)->getOpcode() == BO_Comma))) {
          R.Plan.reject(
              "init-list: conditional evaluation requires a local block");
          return true;
        }
        Node = Parent;
      } else if (auto *Variable = Parents[0].get<VarDecl>()) {
        auto Owners = R.AST.getParents(*Variable);
        if (Owners.size() != 1 || !Owners[0].get<DeclStmt>()) {
          R.Plan.reject("init-list: unsupported initializer owner");
          return true;
        }
        Node = Owners[0].get<DeclStmt>();
      } else {
        R.Plan.reject("init-list: unsupported evaluation context");
        return true;
      }
    }
    if (E->getBeginLoc().isMacroID() || E->getEndLoc().isMacroID()) {
      R.Plan.reject("init-list: macro construction requires review");
      return true;
    }
    std::string Element, Type;
    auto *T = dyn_cast<ClassTemplateSpecializationDecl>(C);
    if (Name == "mlir::ValueRange" || Name == "mlir::TypeRange") {
      Element = Name == "mlir::ValueRange" ? "mlir::Value" : "mlir::Type";
      Type = Name;
    } else if (T && T->getTemplateArgs().size() >= 1) {
      auto Q = T->getTemplateArgs()[0].getAsType();
      if (Name == "llvm::SmallVector") {
        auto *Record = Q->getAsCXXRecordDecl();
        if (!Record ||
            Record->getQualifiedNameAsString() != "mlir::OpFoldResult" ||
            T->getTemplateArgs().size() != 2 ||
            T->getTemplateArgs()[1].getAsIntegral() != 3)
          return true;
        Element = "mlir::OpFoldResult";
        Type = "llvm::SmallVector<mlir::OpFoldResult, 3>";
      } else {
        if (R.AST.hasSameType(Q, R.AST.LongTy))
          Element = "int64_t";
        else if (Transpose && R.AST.hasSameType(Q, R.AST.IntTy))
          Element = "int";
        else if (Transpose && R.AST.hasSameType(Q, R.AST.BoolTy))
          Element = "bool";
        else if (Transpose && Q->getAsCXXRecordDecl()) {
          Element = Q->getAsCXXRecordDecl()->getQualifiedNameAsString();
          if (Element != "mlir::AffineExpr" &&
              Element != "mlir::NamedAttribute" && Element != "mlir::Type" &&
              Element != "mlir::Value" && Element != "mlir::Attribute")
            return true;
        } else
          return true;
        Type = "llvm::ArrayRef<" + Element + ">";
      }
    } else
      return true;
    std::string Values;
    for (auto *Value : L->inits()) {
      ElementCalls Check(Transpose);
      Check.TraverseStmt(Value);
      auto Facts = inspectBody(Value);
      if (!Check.Valid || Facts.HasJump || Facts.Unevaluated ||
          Facts.NestedLambda || Value->getBeginLoc().isMacroID() ||
          Value->getEndLoc().isMacroID()) {
        R.Plan.reject("init-list: element is outside the audited read-only "
                      "query family: " +
                      R.sourceText(Value->getSourceRange()) + " [" +
                      Check.RejectedCall + "]");
        return true;
      }
      if (!Values.empty())
        Values += ", ";
      Values += R.sourceText(Value->getSourceRange());
    }
    auto Array = R.freshName("listValues");
    auto Count = std::to_string(L->getNumInits());
    Preamble += Element + " " + Array + "[] = {" + Values + "};\n" +
                R.indentation(Anchor->getBeginLoc());
    std::string Replacement;
    if (Name == "mlir::ValueRange" || Name == "mlir::TypeRange")
      Replacement = Type + "(llvm::ArrayRef<" + Element + ">(" + Array + ", " +
                    Count + "))";
    else if (Name == "llvm::SmallVector")
      Replacement = Type + "(" + Array + ", " + Array + " + " + Count + ")";
    else
      Replacement = Type + "(" + Array + ", " + Count + ")";
    // For an implicit ArrayRef construction its source is just {values}; a
    // written ValueRange/SmallVector construction includes the type spelling.
    R.Plan.add(R.SM, E->getBeginLoc(), R.tokenEnd(E->getEndLoc()), "init-list",
               Replacement,
               "Select existing array/range constructors with uniquely named "
               "element storage",
               "Elements remain alive in the original enclosing block; no fake "
               "arity overloads");
    return true;
  }
  void finish() {
    if (!Preamble.empty())
      R.Plan.addInsertion(
          R.SM, Anchor->getBeginLoc(), "init-list-preamble", Preamble, 10,
          "Own list elements through the synchronous builder call",
          "Existing ArrayRef/ValueRange/SmallVector constructors");
  }
};
} // namespace
void planInitializerListRewrite(Stmt *S, RewriteContext &R) {
  if (!R.Options.enabled("init-list") ||
      (R.Options.Profile != "depthwise-conv" &&
       R.Options.Profile != "builtin-transpose") ||
      !R.isMainFile(S->getBeginLoc()))
    return;
  auto P = R.AST.getParents(*S);
  if (P.size() != 1 || !P[0].get<CompoundStmt>() || isa<CompoundStmt>(S))
    return;
  // A control statement's children have their own block anchors. Never hoist a
  // conditional/loop initializer outside its execution path.
  if (!isa<DeclStmt, Expr>(S))
    return;
  Lists V(R, S);
  V.TraverseStmt(S);
  V.finish();
}
} // namespace buddy::framac
