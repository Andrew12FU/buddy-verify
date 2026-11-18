// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_OPERATION_H
#define BUDDY_MINI_MLIR_IR_OPERATION_H

#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/Region.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Visitors.h"
#include "llvm/Support/TrailingObjects.h"

// Provenance: audited Reshape V2 mini, line 1214; see docs/PROVENANCE.md.
namespace mlir {
class alignas(8) Operation final
    : public llvm::ilist_node_with_parent<Operation, Block>,
      private llvm::TrailingObjects<Operation, detail::OperandStorage,
                                    detail::OpProperties, BlockOperand, Region,
                                    OpOperand> {
public:
   
   
  OpResult getResult(unsigned idx);
  Value getOperand(unsigned idx);
   
  void setOperand(unsigned idx,Value value);
  llvm::MutableArrayRef<OpOperand> getOpOperands();
  MLIRContext *getContext(); Location getLoc(); void erase();
  InFlightDiagnostic emitOpError(const llvm::Twine &message);
  using result_range = ResultRange;
  using use_range = result_range::use_range;
   
  template <WalkOrder Order = WalkOrder::PostOrder,
            typename Iterator = ForwardIterator, typename FnT,
            typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 1,
                   RetT>
  walk(FnT &&callback) {
    return detail::walk<Order, Iterator>(this, std::forward<FnT>(callback));
  }
  template <typename FnT, typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 2,
                   RetT>
  walk(FnT &&callback);

private:
  Operation(Location location, OperationName name, unsigned numResults,
            unsigned numSuccessors, unsigned numRegions,
            int propertiesStorageSize, DictionaryAttr attributes,
            PropertyRef properties, bool hasOperandStorage);
  ~Operation();
  // Operation's IR storage is outside pointer-only APIs and not a layout model.
};
} // namespace mlir

#endif
