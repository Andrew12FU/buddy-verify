// SPDX-License-Identifier: Apache-2.0
#include "rules/InitializerListRule.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/QualTypeNames.h"
#include "llvm/Support/raw_ostream.h"
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
         Name == "llvm::ArrayRef" || Name == "llvm::SmallVector") &&
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
        if (isa<ConditionalOperator, BinaryConditionalOperator,
                UnaryExprOrTypeTraitExpr, CXXNoexceptExpr, CXXTypeidExpr>(S) ||
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
/// Inspect list elements without rewriting their expressions. Only the reviewed
/// handle query/conversion family is accepted; arbitrary side effects reject.
class ElementCalls : public RecursiveASTVisitor<ElementCalls> {
public:
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
    if (isa<CXXConversionDecl>(D) && M &&
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
  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    auto *D = E->getConstructor();
    const std::set<std::string> Handles{
        "mlir::Value", "mlir::Type", "mlir::AffineExpr", "mlir::Attribute",
        "mlir::NamedAttribute", "mlir::OpFoldResult", "llvm::ArrayRef",
        "llvm::SmallVector", "mlir::ValueRange", "mlir::TypeRange", "llvm::StringRef"};
    if (!D->isTrivial() && !Handles.count(D->getParent()->getQualifiedNameAsString())) {
      Valid = false;
      RejectedCall = D->getQualifiedNameAsString();
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
    if (Name != "mlir::ValueRange" &&
        Name != "mlir::TypeRange" &&
        Name != "llvm::SmallVector" && Name != "llvm::ArrayRef")
      return true;
    // Only a written brace construction. Constructors around an existing named
    // range or pointer/length pair have no initializer list and are idempotent.
    auto *L = ownList(E);
    if (!L || !L->getNumInits())
      return true;
    for (auto *Value : L->inits())
      if (hasNestedRangeList(Value))
        return true;
    // Hoisting out of a conditional operand would execute a previously skipped
    // query. Stop at nested callbacks and reject
    // conditional/comma/short-circuit contexts instead of silently changing
    // their evaluation path.
    if (!unconditionalPaths(E, Anchor, R)) {
      R.Plan.reject("init-list: all nested-list parent paths must be unconditional in the original block statement");
      return true;
    }
    if (E->getBeginLoc().isMacroID() || E->getEndLoc().isMacroID()) {
      R.Plan.reject("init-list: macro construction requires review");
      return true;
    }
    std::string Element, Type;
    QualType ArrayElement;
    auto *T = dyn_cast<ClassTemplateSpecializationDecl>(C);
    if (Name == "mlir::ValueRange" || Name == "mlir::TypeRange") {
      Element = Name == "mlir::ValueRange" ? "mlir::Value" : "mlir::Type";
      Type = Name;
    } else if (T && T->getTemplateArgs().size() >= 1 &&
               T->getTemplateArgs()[0].getKind() == TemplateArgument::Type) {
      auto Q = T->getTemplateArgs()[0].getAsType();
      // The type family supplies existing array/iterator constructors. The
      // element and inline capacity are derived from AST, not a pass fixture.
      const std::set<std::string> AuditedRecords{
          "mlir::Value", "mlir::Type", "mlir::AffineExpr", "mlir::Attribute",
          "mlir::NamedAttribute", "mlir::OpFoldResult"};
      auto *ElementRecord = Q->getAsCXXRecordDecl();
      bool Audited = ElementRecord &&
                     AuditedRecords.count(ElementRecord->getQualifiedNameAsString());
      if (!(Q->isArithmeticType() || Q->isEnumeralType() ||
            Q->isPointerType() ||
            (Q->isRecordType() && (Audited || Q.isTriviallyCopyableType(R.AST)))))
        return true;
      PrintingPolicy Policy(R.Lang);
      Policy.SuppressTagKeyword = true;
      ArrayElement = Q;
      Element = TypeName::getFullyQualifiedName(Q, R.AST, Policy);
      Type = TypeName::getFullyQualifiedName(E->getType(), R.AST, Policy);
      if (Name == "llvm::ArrayRef")
        Type = "llvm::ArrayRef<" + Element + ">";
    } else
      return true;
    std::string Values;
    for (auto *Value : L->inits()) {
      ElementCalls Check;
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
    std::string Declaration = Element + " " + Array + "[]";
    if (!ArrayElement.isNull()) {
      Declaration.clear();
      llvm::raw_string_ostream Stream(Declaration);
      PrintingPolicy Policy(R.Lang);
      Policy.SuppressTagKeyword = true;
      TypeName::getFullyQualifiedType(ArrayElement, R.AST).print(
          Stream, Policy, Array + "[]");
      Stream.flush();
    }
    Preamble += Declaration + " = {" + Values + "};\n" +
                R.indentation(Anchor->getBeginLoc());
    std::string Replacement;
    if (Name == "mlir::ValueRange" || Name == "mlir::TypeRange")
      Replacement = Type + "(llvm::ArrayRef<" + Element + ">(" + Array + ", " +
                    Count + "))";
    else if (Name == "llvm::SmallVector")
      Replacement = Type + "(" + Array + ", " + Array + " + " + Count + ")";
    else
      Replacement = Type + "(" + Array + ", " + Count + ")";
    // In a direct-list declaration `Range name{...}`, Clang's constructor
    // range starts at the declarator name, not the type. Keep that declarator
    // while replacing its constructor arguments; expression constructions keep
    // the fully qualified type prefix.
    const Stmt *Owner = E;
    while (true) {
      auto Parents = R.AST.getParents(*Owner);
      if (Parents.size() != 1)
        break;
      if (auto *V = Parents[0].get<VarDecl>()) {
        if (V->getLocation() == E->getBeginLoc())
          Replacement = V->getNameAsString() + Replacement.substr(Type.size());
        break;
      }
      auto *P = Parents[0].get<Expr>();
      if (!P || !isa<ExprWithCleanups, MaterializeTemporaryExpr,
                     CXXBindTemporaryExpr, ImplicitCastExpr>(P))
        break;
      Owner = P;
    }
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
  if (!R.Options.enabled("init-list") || !R.isMainFile(S->getBeginLoc()))
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
