// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "support/AstSupport.h"
namespace buddy::framac {
/// Replace an audited const handle initialization with mutable storage and a
/// const reference of the original name. D is a single local declaration; R
/// owns the edit plan. Returns nothing; unsupported shapes reject the plan.
void planConstLocalRewrite(clang::DeclStmt *D, RewriteContext &R);
} // namespace buddy::framac
