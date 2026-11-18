// SPDX-License-Identifier: Apache-2.0
#include "rules/NamespaceRule.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Path.h"

using namespace clang;
namespace buddy::framac {
// canonical 声明将同一匿名空间的多次重开归为一组；编号按原文扫描顺序生成。
NamespaceRule::NamespaceRule(RewriteContext &R) : R(R) {
  std::string Prefix = R.Options.NamespacePrefix;
  if (Prefix.empty())
    Prefix = llvm::sys::path::stem(R.Plan.Input).str();
  for (char &Ch : Prefix)
    if (!llvm::isAlnum(Ch) && Ch != '_')
      Ch = '_';
  if (Prefix.empty() || llvm::isDigit(Prefix.front()))
    Prefix = "pass_" + Prefix;
  for (auto *NS : R.Inventory.Anonymous) {
    auto *Canonical = NS->getCanonicalDecl();
    if (!NamespaceNames.count(Canonical)) {
      unsigned N = NamespaceNames.size() + 1;
      NamespaceNames[Canonical] =
          R.freshName(Prefix + "_anonymousNamespace_" + (N < 10 ? "0" : "") +
                      std::to_string(N));
    }
  }
}

void NamespaceRule::planDeclaration(NamespaceDecl *D) {
  if (!R.Options.enabled("namespace") || !D->isAnonymousNamespace() ||
      !R.isMainFile(D->getBeginLoc()))
    return;
  auto L = D->getBeginLoc();
  R.Plan.add(
      R.SM, L, R.tokenEnd(L), "namespace",
      "namespace " + NamespaceNames[D->getCanonicalDecl()],
      "Name an anonymous namespace; reopened declarations share one name",
      "Changes internal linkage; single-TU compatibility workaround");
  return;
}

void NamespaceRule::planTypeReference(RecordTypeLoc T) {
  if (!R.Options.enabled("namespace") || !R.isMainFile(T.getNameLoc()))
    return;
  auto *Owner = R.anonymousOwner(T.getDecl()->getDeclContext());
  // 内部宏引用不需要限定；外部宏展开不可直接编辑，必须明确拒绝。
  if (T.getNameLoc().isMacroID()) {
    if (Owner && NamespaceNames.count(Owner) &&
        !R.insideNamespace(R.SM.getExpansionLoc(T.getNameLoc()), Owner))
      R.Plan.reject(
          "namespace: external macro-generated type reference needs review");
    return;
  }
  if (Owner && NamespaceNames.count(Owner) &&
      !R.insideNamespace(T.getNameLoc(), Owner))
    R.Plan.add(
        R.SM, T.getNameLoc(), R.tokenEnd(T.getNameLoc()), "namespace-reference",
        NamespaceNames[Owner] + "::" + R.sourceText(T.getSourceRange()),
        "Preserve the bound class reference outside the renamed namespace");
  return;
}

} // namespace buddy::framac
