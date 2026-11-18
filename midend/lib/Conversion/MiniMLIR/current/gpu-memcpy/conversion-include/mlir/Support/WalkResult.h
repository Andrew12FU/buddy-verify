// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_SUPPORT_WALKRESULT_H
#define BUDDY_MINI_MLIR_SUPPORT_WALKRESULT_H
#include "mlir/Support/LLVM.h"
namespace mlir {
class WalkResult {
 enum Result { Interrupt,Advance,Skip };
 Result result;
public:
 static WalkResult advance();    
  
};
}

#endif
