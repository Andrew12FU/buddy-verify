#ifndef FC_LLVM_ADT_ITERATOR_RANGE_H
#define FC_LLVM_ADT_ITERATOR_RANGE_H

namespace llvm {

/* 官方 llvm::iterator_range（23.x 在 ADT/iterator_range.h，STLExtras.h 只 include）。
 * 模型只保留 pass 用到的 begin/end/empty。 */
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