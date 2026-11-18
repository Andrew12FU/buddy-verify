// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_RANGE_FOR_RULE_H
#define BUDDY_FRAMAC_RANGE_FOR_RULE_H
#include "support/AstSupport.h"
namespace buddy::framac {
/// Desugar one outer range-for tree, retaining nested bodies and loop targets.
/// @param S Candidate main-file loop. @param R Borrowed AST and output plan.
/// @return Nothing; unsupported ranges register a rejection, never partial output.
void planRangeForRewrite(clang::CXXForRangeStmt *S, RewriteContext &R);
}
#endif
