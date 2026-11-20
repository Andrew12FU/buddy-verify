// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_VALUERANGERULE_H
#define BUDDY_FRAMAC_VALUERANGERULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// Materialize list-backed ValueRange call arguments using their AST types.
/// Accepts a direct block expression or single automatic declaration; no fixed
/// callee, argument slots, number of lists, callback position or pass profile.
/// Other argument effects and handle conversion/lifetime constraints are checked.
void planValueRangeRewrite(clang::CallExpr *Call, RewriteContext &R);
} // namespace buddy::framac
#endif
