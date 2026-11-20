#ifndef FC_LLVM_ADT_STLEEXTRAS_H
#define FC_LLVM_ADT_STLEEXTRAS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

/* 官方 llvm::iterator_range 就在这个头里。
 * 官方的 enumerate / make_early_inc_range / zip / filter 等四个 pass 都不用，
 * 按"只定义用到的"只保留 iterator_range（use_range / user_range 需要）。 */
template <typename IteratorT> class iterator_range {
public:
  typedef IteratorT iterator;

  iterator_range();
  iterator_range(IteratorT begin, IteratorT end)
      : begin_iterator(begin), end_iterator(end) {}
  template <typename Container>
  iterator_range(Container &&c)
      : begin_iterator(c.begin()), end_iterator(c.end()) {}

  iterator begin() const { return begin_iterator; }
  iterator end() const { return end_iterator; }
  bool empty() const { return begin_iterator == end_iterator; }

private:
  IteratorT begin_iterator;
  IteratorT end_iterator;
};

} // namespace llvm
#endif
