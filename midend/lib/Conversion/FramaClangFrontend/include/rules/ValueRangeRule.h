// SPDX-License-Identifier: Apache-2.0
#ifndef BUDDY_FRAMAC_VALUERANGERULE_H
#define BUDDY_FRAMAC_VALUERANGERULE_H

#include "support/AstSupport.h"

namespace buddy::framac {
/// 将 SiLU GenericOp::create 的两个 ValueRange 列表改为具名数组和 ArrayRef。
/// @param Call 当前调用；只处理单变量声明初始化中的已审计 GenericOp 调用。
/// @param R 共享上下文；数组插入与捕获指针通过显式顺序协作。
/// @return 无；拒绝空列表、非局部对象元素、宏以及可能改变求值时机的实参。
void planValueRangeRewrite(clang::CallExpr *Call, RewriteContext &R);
} // namespace buddy::framac
#endif
