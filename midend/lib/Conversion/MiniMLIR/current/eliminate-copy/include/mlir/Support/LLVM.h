// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_SUPPORT_LLVM_H
#define BUDDY_MINI_MLIR_SUPPORT_LLVM_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/LogicalResult.h"

// Provenance: audited Reshape V2 mini, line 827; see docs/PROVENANCE.md.
namespace mlir {
using llvm::ArrayRef;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 828; see docs/PROVENANCE.md.
namespace mlir {
using llvm::SmallVector;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 829; see docs/PROVENANCE.md.
namespace mlir {
using llvm::dyn_cast;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 830; see docs/PROVENANCE.md.
namespace mlir {
using LogicalResult = llvm::LogicalResult;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 831; see docs/PROVENANCE.md.
namespace mlir {
using StringRef = llvm::StringRef;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 832; see docs/PROVENANCE.md.
namespace mlir {
using llvm::success;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 833; see docs/PROVENANCE.md.
namespace mlir {
using llvm::failure;
} // namespace mlir

namespace mlir {
using llvm::cast;
using llvm::failed;
using llvm::function_ref;
} // namespace mlir
namespace mlir { using llvm::isa; }
namespace mlir { using llvm::dyn_cast_or_null; }
#endif
