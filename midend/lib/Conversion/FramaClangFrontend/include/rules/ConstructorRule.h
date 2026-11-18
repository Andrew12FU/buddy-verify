// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_CONSTRUCTOR_RULE_H
#define BUDDY_FRAMAC_CONSTRUCTOR_RULE_H
#include "support/AstSupport.h"
namespace buddy::framac {
/// Add the audited context forwarding ctor while retaining inherited overloads.
/// @param D Candidate using declaration; @param R borrowed AST/output context.
/// @return Nothing; unsupported shapes reject the whole rewrite plan.
void planConstructorRewrite(clang::UsingDecl *D, RewriteContext &R);
}
#endif
