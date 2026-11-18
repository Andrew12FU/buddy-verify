// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations; see docs/BUILTIN_TRANSPOSE.md.
#pragma once
#include "mlir/Dialect/Arith/IR/Arith.h"
namespace mlir { namespace affine {
struct LoopReduction { arith::AtomicRMWKind kind; unsigned iterArgPosition; Value value; };
} }
