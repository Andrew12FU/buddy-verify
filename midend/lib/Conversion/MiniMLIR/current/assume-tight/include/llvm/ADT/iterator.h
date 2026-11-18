// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BUDDY_MINI_LLVM_ADT_ITERATOR_H
#define BUDDY_MINI_LLVM_ADT_ITERATOR_H
#include "mini/Support/Standard.h"
namespace llvm {
/// Declaration boundary for iterator mixins; algorithms remain external.
/// DerivedT is the concrete iterator, category and T retain actual type identity.
template<typename DerivedT, typename IteratorCategoryT, typename T,
         typename DifferenceTypeT = std::ptrdiff_t, typename PointerT = T *,
         typename ReferenceT = T &>
class iterator_facade_base {
public:
  using iterator_category = IteratorCategoryT;
  using value_type = T;
  using difference_type = DifferenceTypeT;
  using pointer = PointerT;
  using reference = ReferenceT;
  bool operator!=(const DerivedT &RHS) const;
  DerivedT &operator++();
};
}
namespace llvm {
template<typename DerivedT, typename WrappedIteratorT,
 typename IteratorCategoryT=typename std::iterator_traits<WrappedIteratorT>::iterator_category,
 typename T=typename std::iterator_traits<WrappedIteratorT>::value_type,
 typename DifferenceTypeT=typename std::iterator_traits<WrappedIteratorT>::difference_type,
 typename PointerT=std::conditional_t<std::is_same<T, typename std::iterator_traits<WrappedIteratorT>::value_type>::value, typename std::iterator_traits<WrappedIteratorT>::pointer, T *>,
 typename ReferenceT=std::conditional_t<std::is_same<T, typename std::iterator_traits<WrappedIteratorT>::value_type>::value, typename std::iterator_traits<WrappedIteratorT>::reference, T &>>
class iterator_adaptor_base : public iterator_facade_base<DerivedT,IteratorCategoryT,T,DifferenceTypeT,PointerT,ReferenceT> {
protected: WrappedIteratorT I;
public:
 explicit iterator_adaptor_base(WrappedIteratorT u);
  
 friend bool operator==(const iterator_adaptor_base &LHS,const iterator_adaptor_base &RHS);
};
}

#endif
