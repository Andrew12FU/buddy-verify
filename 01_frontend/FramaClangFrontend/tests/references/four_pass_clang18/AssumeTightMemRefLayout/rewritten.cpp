//===- AssumeTightMemRefLayout.cpp ---------------------------------------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
//
// This file implements a helper pass that tightens memref values with a
// trailing dynamic stride to an explicitly unit-stride layout by reinterpreting
// the underlying buffer.
//
// NOTE: this copy is syntactically adapted for the Frama-Clang front-end.
//       Range-based for loops and structured bindings (unsupported by
//       Frama-Clang 0.0.20) are desugared into equivalent indexed loops.
//       The semantics of the pass are unchanged.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace assume_tight_detail {

/// Returns true if `type` is a memref whose layout uses a trailing dynamic
/// stride that we want to tighten to 1.
static bool hasLooseTrailingStride(MemRefType type) {
  if (!type.hasRank() || type.getRank() == 0)
    return false;
  auto layout = dyn_cast<StridedLayoutAttr>(type.getLayout());
  if (!layout)
    return false;
  ArrayRef<int64_t> strides = layout.getStrides();
  if (strides.empty())
    return false;
  int64_t trailingStride = strides.back();
  if (trailingStride == 1)
    return false;
  return ShapedType::isDynamic(trailingStride);
}

class AssumeTightMemRefLayoutPass
    : public PassWrapper<AssumeTightMemRefLayoutPass,
                         OperationPass<func::FuncOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AssumeTightMemRefLayoutPass)

  StringRef getArgument() const final { return "assume-tight-memref-layout"; }

  StringRef getDescription() const final {
    return "Assume trailing unit stride for eligible memrefs by tightening "
           "their layout with reinterpret_cast.";
  }

  void runOnOperation() override {
    func::FuncOp func = getOperation();
    SmallVector<Value> worklist;

    // Collect block arguments (including entry block) first.
    Region &funcBody = func.getBody();
    for (Block *blockIt = funcBody.begin(); blockIt != funcBody.end();
         ++blockIt) {
      Block &block = *blockIt;
      llvm::ArrayRef<BlockArgument> args = block.getArguments();
      for (size_t argIndex = 0; argIndex < args.size(); ++argIndex) {
        BlockArgument arg = args[argIndex];
        if (isa<MemRefType>(arg.getType()))
          worklist.push_back(arg);
      }
    }

    // Collect op results.  Frama-Clang cannot capture a class-typed local by
    // reference, so the worklist is reached through an explicit pointer.
    SmallVector<Value> *worklistPtr = &worklist;
    func.walk([worklistPtr](Operation *op) {
      ResultRange results = op->getResults();
      for (size_t resultIndex = 0; resultIndex < results.size(); ++resultIndex) {
        Value result = results[resultIndex];
        if (isa<MemRefType>(result.getType()))
          worklistPtr->push_back(result);
      }
    });

    for (size_t valueIndex = 0; valueIndex < worklist.size(); ++valueIndex) {
      Value value = worklist[valueIndex];
      auto type = dyn_cast<MemRefType>(value.getType());
      if (!type || !hasLooseTrailingStride(type))
        continue;

      SmallVector<OpOperand *> toUpdate;
      use_range uses = value.getUses();
            use_range::iterator useIt = uses.begin();
            use_range::iterator useItEnd = uses.end();
       for (; useIt != useItEnd; ++useIt) {
        OpOperand &use = *useIt;
        Operation *owner = use.getOwner();
        if (isa<func::ReturnOp>(owner))
          continue;
        if (isa<CallOpInterface>(owner))
          continue;
        if (isa<BranchOpInterface>(owner))
          continue;
        if (isa<RegionBranchOpInterface>(owner))
          continue;
        toUpdate.push_back(&use);
      }

      if (toUpdate.empty())
        continue;

      OpBuilder builder(func.getContext());
      if (Operation *def = value.getDefiningOp())
        builder.setInsertionPointAfter(def);
      else
        builder.setInsertionPointToStart(value.getParentBlock());

      // Extract the current layout metadata so we can rebuild a tighter view.
      auto metadata = memref::ExtractStridedMetadataOp::create(
          builder, value.getLoc(), value);

      SmallVector<OpFoldResult> sizes;
      sizes.reserve(type.getRank());
      auto metadataSizes = metadata.getSizes();
      ArrayRef<int64_t> shape = type.getShape();
      for (size_t idx = 0; idx < shape.size(); ++idx) {
        int64_t dim = shape[idx];
        if (ShapedType::isDynamic(dim))
          sizes.push_back(metadataSizes[idx]);
        else
          sizes.push_back(builder.getIndexAttr(dim));
      }

      auto layout = cast<StridedLayoutAttr>(type.getLayout());
      ArrayRef<int64_t> strides = layout.getStrides();

      SmallVector<OpFoldResult> stridesOfr;
      stridesOfr.reserve(type.getRank());
      auto metadataStrides = metadata.getStrides();
      for (size_t idx = 0; idx < strides.size(); ++idx) {
        int64_t stride = strides[idx];
        if (idx == static_cast<int64_t>(type.getRank()) - 1) {
          stridesOfr.push_back(builder.getIndexAttr(1));
          continue;
        }
        if (ShapedType::isDynamic(stride))
          stridesOfr.push_back(metadataStrides[idx]);
        else
          stridesOfr.push_back(builder.getIndexAttr(stride));
      }

      OpFoldResult offset;
      if (ShapedType::isDynamic(layout.getOffset()))
        offset = metadata.getOffset();
      else
        offset = builder.getIndexAttr(layout.getOffset());

      SmallVector<int64_t> tightenedStrides(strides.begin(), strides.end());
      tightenedStrides.back() = 1;
      auto tightenedLayout = StridedLayoutAttr::get(
          type.getContext(), layout.getOffset(), tightenedStrides);
      auto tightenedType =
          MemRefType::get(type.getShape(), type.getElementType(),
                          tightenedLayout, type.getMemorySpace());

      auto reinterpret = memref::ReinterpretCastOp::create(
          builder, value.getLoc(), tightenedType, metadata.getBaseBuffer(),
          offset, sizes, stridesOfr);

      for (size_t operandIndex = 0; operandIndex < toUpdate.size();
           ++operandIndex) {
        OpOperand *operand = toUpdate[operandIndex];
        operand->set(reinterpret);
      }
    }
  }
};

} // namespace assume_tight_detail

namespace mlir {
namespace buddy {

void registerAssumeTightMemRefLayoutPass() {
  PassRegistration<assume_tight_detail::AssumeTightMemRefLayoutPass>();
}

} // namespace buddy
} // namespace mlir
