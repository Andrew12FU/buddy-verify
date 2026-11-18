// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations from the local LLVM/MLIR checkout.
#ifndef BUDDY_MINI_LLVM_ADT_ITERATOR_RANGE_H
#define BUDDY_MINI_LLVM_ADT_ITERATOR_RANGE_H
#include "mini/Support/Standard.h"
namespace llvm {
template <typename IteratorT> class iterator_range {
  IteratorT begin_iterator, end_iterator;
public:
  iterator_range(IteratorT begin_iterator, IteratorT end_iterator);
  IteratorT begin() const;
  IteratorT end() const;
};
}

#endif
