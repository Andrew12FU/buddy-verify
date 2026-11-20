// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_IR_TYPERANGE_H
#define BUDDY_MINI_MLIR_IR_TYPERANGE_H
#include "mlir/IR/ValueRange.h"
namespace mlir {
 







}


#include "llvm/ADT/iterator_range.h"
namespace mlir {
template<typename ValueIteratorT>
class ValueTypeIterator final : public llvm::mapped_iterator_base<ValueTypeIterator<ValueIteratorT>,ValueIteratorT,Type> {
public: using llvm::mapped_iterator_base<ValueTypeIterator<ValueIteratorT>,ValueIteratorT,Type>::mapped_iterator_base;
  
};
template<typename ValueRangeT>
class ValueTypeRange final : public llvm::iterator_range<ValueTypeIterator<typename ValueRangeT::iterator>> {
public: using llvm::iterator_range<ValueTypeIterator<typename ValueRangeT::iterator>>::iterator_range;
};
}

#endif
