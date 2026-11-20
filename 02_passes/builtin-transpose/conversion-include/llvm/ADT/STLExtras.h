// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_STLEXTRAS_H
#define BUDDY_MINI_LLVM_ADT_STLEXTRAS_H

#include "mini/Support/Standard.h"
#include "llvm/ADT/iterator.h"

// Provenance: audited Reshape V2 mini, line 243; see docs/PROVENANCE.md.
namespace llvm {
 

} // namespace llvm
// Provenance: audited Reshape V2 mini, line 245; see docs/PROVENANCE.md.
namespace llvm {
 






} // namespace llvm
// Provenance: audited Reshape V2 mini, line 251; see docs/PROVENANCE.md.
namespace llvm {
 


} // namespace llvm
// Provenance: audited Reshape V2 mini, line 253; see docs/PROVENANCE.md.
namespace llvm {
 






} // namespace llvm
// Provenance: audited Reshape V2 mini, line 259; see docs/PROVENANCE.md.
namespace llvm {
 


} // namespace llvm
// Provenance: audited Reshape V2 mini, line 261; see docs/PROVENANCE.md.
namespace llvm {
 


} // namespace llvm
// Provenance: audited Reshape V2 mini, line 263; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, typename... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 265; see docs/PROVENANCE.md.
namespace llvm {
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 292; see docs/PROVENANCE.md.
namespace llvm {
namespace detail {
template <class, template <class...> class Op, class... Args> struct detector {
  using value_t = std::false_type;
};
} // namespace detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 293; see docs/PROVENANCE.md.
namespace llvm {
namespace detail {
template <template <class...> class Op, class... Args>
struct detector<std::void_t<Op<Args...>>, Op, Args...> {
  using value_t = std::true_type;
};
} // namespace detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 296; see docs/PROVENANCE.md.
namespace llvm {
template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<void, Op, Args...>::value_t;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 612; see docs/PROVENANCE.md.
namespace llvm {
template <typename T> struct Repeated;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 613; see docs/PROVENANCE.md.
namespace llvm {
template <typename IteratorT> class iterator_range;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 615; see docs/PROVENANCE.md.
namespace llvm {
template <typename... Ts> struct TypesAreDistinct;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 616; see docs/PROVENANCE.md.
namespace llvm {
template <> struct TypesAreDistinct<> : std::true_type {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 617; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, typename... Us>
struct TypesAreDistinct<T, Us...>
    : std::conjunction<std::negation<is_one_of<T, Us...>>,
                       TypesAreDistinct<Us...>> {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 652; see docs/PROVENANCE.md.
namespace llvm {
template<typename DerivedT,typename BaseT,typename T,typename PointerT=T *,typename ReferenceT=T &>
class indexed_accessor_iterator : public llvm::iterator_facade_base<DerivedT,std::random_access_iterator_tag,T,std::ptrdiff_t,PointerT,ReferenceT> {
public:    
protected: indexed_accessor_iterator(BaseT base,std::ptrdiff_t index);
 BaseT base; std::ptrdiff_t index;
};
namespace detail {
template <typename DerivedT, typename BaseT, typename T,
          typename PointerT = T *, typename ReferenceT = T &>
class indexed_accessor_range_base {
public:
  using RangeBaseT = indexed_accessor_range_base;
   
  ReferenceT front() const;
  class iterator : public indexed_accessor_iterator<iterator,BaseT,T,PointerT,ReferenceT> {
  public:
    ReferenceT operator*() const;
  private:
    iterator(BaseT owner,std::ptrdiff_t curIndex);
    friend indexed_accessor_range_base<DerivedT,BaseT,T,PointerT,ReferenceT>;
  };
  iterator begin() const;
  iterator end() const;
  inline indexed_accessor_range_base(iterator begin, iterator end);
  inline indexed_accessor_range_base(const iterator_range<iterator> &range);
  inline indexed_accessor_range_base(BaseT base, long count);

protected:
  indexed_accessor_range_base(const indexed_accessor_range_base &) = default;
  indexed_accessor_range_base(indexed_accessor_range_base &&) = default;
  indexed_accessor_range_base &
  operator=(const indexed_accessor_range_base &) = default;
  BaseT base;
  long count;
};
} // namespace detail
} // namespace llvm

// Original-source syntax check: declaration-only, one-range enumerate surface.
// Rewritten passes use their underlying indexed range; no enumerate algorithm
// or tuple object layout is assigned a behavioral model here.
namespace llvm { namespace detail {
struct index_iterator : llvm::iterator_facade_base<index_iterator, std::random_access_iterator_tag, size_t> {
  size_t Index;
  size_t operator*() const;
};
struct index_stream { index_iterator begin() const; index_iterator end() const; };
template<typename... Refs> struct enumerator_result;
template<typename... Refs> struct enumerator_result<size_t, Refs...> {
  using value_reference_tuple = std::tuple<size_t, Refs...>;
  static constexpr size_t NumValues = sizeof...(Refs) + 1;
};
 


template<typename... Iters> struct zip_enumerator {
  using value_type = enumerator_result<decltype(*std::declval<Iters>())...>;
  value_type operator*() const;
   
   
};
template<template<typename...> class ItType, typename... Args> class zippy;
template<template<typename...> class ItType, typename Range>
class zippy<ItType, index_stream, Range> {
public:
  using iterator = ItType<index_iterator, decltype(std::declval<Range &>().begin())>;
  iterator begin();
  iterator end();
};
}
 


}
namespace std {
 
 
 

}
namespace llvm {
template<typename DerivedT,typename ItTy,typename ReferenceTy>
class mapped_iterator_base : public iterator_adaptor_base<DerivedT,ItTy,typename std::iterator_traits<ItTy>::iterator_category,std::remove_reference_t<ReferenceTy>,typename std::iterator_traits<ItTy>::difference_type,std::remove_reference_t<ReferenceTy> *,ReferenceTy> {
public:
 using BaseT=mapped_iterator_base;
 mapped_iterator_base(ItTy U);
 ReferenceTy operator*() const;
};
template<typename WrappedIteratorT>
class early_inc_iterator_impl : public iterator_adaptor_base<early_inc_iterator_impl<WrappedIteratorT>,WrappedIteratorT,std::input_iterator_tag> {
 using BaseT=typename early_inc_iterator_impl::iterator_adaptor_base;
public:
 early_inc_iterator_impl(WrappedIteratorT I);
 decltype(*std::declval<WrappedIteratorT>()) operator*();
  
};
namespace detail {   }
 

}


#include "llvm/ADT/iterator_range.h"
namespace llvm {
template<typename ItTy,typename FuncTy,typename ReferenceTy=decltype(std::declval<FuncTy>()(*std::declval<ItTy>()))>
class mapped_iterator : public iterator_adaptor_base<mapped_iterator<ItTy,FuncTy>,ItTy,typename std::iterator_traits<ItTy>::iterator_category,std::remove_reference_t<ReferenceTy>,typename std::iterator_traits<ItTy>::difference_type,std::remove_reference_t<ReferenceTy> *,ReferenceTy> {
public:
 mapped_iterator(ItTy U,FuncTy F):mapped_iterator::iterator_adaptor_base(std::move(U)){}
 // BUDDY_EXTERNAL_TEMPLATE_ADAPTER: local closure constructor is visible;
 // callable storage/dispatch are external, as for function_ref. The empty
 // adapter body must not be interpreted as a no-effect specification.
 ReferenceTy operator*() const;
};
template<typename ContainerTy,typename FuncTy>
auto map_range(ContainerTy &&C,FuncTy F) {
 using It=decltype(C.begin());
 using Mapped=mapped_iterator<It,FuncTy>;
 return iterator_range<Mapped>(Mapped(C.begin(),F),Mapped(C.end(),F));
}
}

#endif
