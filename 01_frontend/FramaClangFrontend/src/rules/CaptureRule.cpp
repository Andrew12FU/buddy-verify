// SPDX-License-Identifier: Apache-2.0
#include "rules/CaptureRule.h"
#include "rules/NestedCapture.h"

namespace buddy::framac {
void planCaptureRewrite(clang::LambdaExpr *L, RewriteContext &R) {
  if (!R.Options.enabled("capture") || !R.isMainFile(L->getBeginLoc()))
    return;
  // 单层也是只有根节点的捕获树；不再根据 profile 选择改写器。
  planCaptureTreeRewrite(L, R);
}
} // namespace buddy::framac
