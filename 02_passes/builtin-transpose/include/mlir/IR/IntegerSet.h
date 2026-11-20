// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations; algorithms require external specifications.
#pragma once
#include "mlir/IR/AffineExpr.h"


namespace mlir { namespace detail { struct IntegerSetStorage; }
class IntegerSet {
public: IntegerSet()=default;
 static IntegerSet get(unsigned dimCount,unsigned symbolCount,ArrayRef<AffineExpr> constraints,ArrayRef<bool> eqFlags);
private: detail::IntegerSetStorage *set=nullptr;
};
}
