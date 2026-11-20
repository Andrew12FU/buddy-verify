// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_ILIST_H
#define BUDDY_MINI_LLVM_ADT_ILIST_H

#include "mlir/Support/Forward.h"
#include "llvm/ADT/ilist_node.h"

// Provenance: audited Reshape V2 mini, line 1113; see docs/PROVENANCE.md.
namespace llvm {
template <bool EnableSentinelTracking, class ParentTy> class ilist_base {
public:
  using node_base_type = ilist_node_base<EnableSentinelTracking, ParentTy>;
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1117; see docs/PROVENANCE.md.
namespace llvm {
template <class OptionsT, bool IsReverse, bool IsConst> class ilist_iterator;
namespace ilist_detail {
template<class OptionsT> struct SpecificNodeAccess;
template<class IteratorTy, class ParentTy, bool IsConst> class iterator_parent_access {};
}
template <class OptionsT, bool IsReverse, bool IsConst> class ilist_iterator
    : ilist_detail::SpecificNodeAccess<OptionsT>,
      public ilist_detail::iterator_parent_access<ilist_iterator<OptionsT, IsReverse, IsConst>, typename OptionsT::parent_ty, IsConst> {
  ilist_node_impl<OptionsT> *NodePtr = nullptr;
public:
  ilist_iterator() = default;
  typename OptionsT::reference operator*() const;
   
   
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1118; see docs/PROVENANCE.md.
namespace llvm {
template <class OptionsT, bool IsReverse, bool IsConst>
class ilist_iterator_w_bits;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1119; see docs/PROVENANCE.md.
namespace llvm {
template <class OptionsT, bool IsReverse, bool IsConst>
using ilist_select_iterator_type =
    std::conditional_t<OptionsT::has_iterator_bits,
                       ilist_iterator_w_bits<OptionsT, IsReverse, IsConst>,
                       ilist_iterator<OptionsT, IsReverse, IsConst>>;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1123; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
struct NodeAccess {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1124; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class OptionsT> struct SpecificNodeAccess : NodeAccess {
protected:
  using pointer = typename OptionsT::pointer;
  using const_pointer = typename OptionsT::const_pointer;
  using node_type = ilist_node_impl<OptionsT>;
   
   
   
   
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1135; see docs/PROVENANCE.md.
namespace llvm {
template <class OptionsT>
class ilist_sentinel : public ilist_node_impl<OptionsT> {
public:
  inline ilist_sentinel();
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1139; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, class... Options>
class simple_ilist
    : ilist_detail::compute_node_options<T, Options...>::type::list_base_type,
      ilist_detail::SpecificNodeAccess<
          typename ilist_detail::compute_node_options<T, Options...>::type> {
  static_assert(ilist_detail::check_options<Options...>::value,
                "Unrecognized node option!");
  using OptionsT =
      typename ilist_detail::compute_node_options<T, Options...>::type;
  using list_base_type = typename OptionsT::list_base_type;
  ilist_sentinel<OptionsT> Sentinel;

public:
  using value_type = typename OptionsT::value_type;
  using pointer = typename OptionsT::pointer;
  using reference = typename OptionsT::reference;
  using const_pointer = typename OptionsT::const_pointer;
  using const_reference = typename OptionsT::const_reference;
  using iterator = ilist_select_iterator_type<OptionsT, false, false>;
  using const_iterator = ilist_select_iterator_type<OptionsT, false, true>;
  using reverse_iterator = ilist_select_iterator_type<OptionsT, true, false>;
  using const_reverse_iterator =
      ilist_select_iterator_type<OptionsT, true, true>;
  using size_type = size_t;
  iterator begin();
  iterator end();
  using difference_type = long; // target canonical ptrdiff_t
  simple_ilist() = default;
  ~simple_ilist() = default;
  simple_ilist(const simple_ilist &) = delete;
  simple_ilist &operator=(const simple_ilist &) = delete;
  inline simple_ilist(simple_ilist &&X);
  inline simple_ilist &operator=(simple_ilist &&X);
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1166; see docs/PROVENANCE.md.
namespace llvm {
template <typename NodeTy> struct ilist_alloc_traits {
   
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1167; see docs/PROVENANCE.md.
namespace llvm {
template <typename NodeTy> struct ilist_callback_traits {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1168; see docs/PROVENANCE.md.
namespace llvm {
template <typename NodeTy>
struct ilist_node_traits : ilist_alloc_traits<NodeTy>,
                           ilist_callback_traits<NodeTy> {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1169; see docs/PROVENANCE.md.
namespace llvm {
template <typename NodeTy>
struct ilist_traits : public ilist_node_traits<NodeTy> {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1170; see docs/PROVENANCE.md.
namespace llvm {
template <typename Ty> struct ilist_traits<const Ty> {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1171; see docs/PROVENANCE.md.
namespace llvm {
template <>
struct ilist_traits<::mlir::Block> : public ilist_alloc_traits<::mlir::Block> {
  using Block = ::mlir::Block;
  using block_iterator = simple_ilist<Block>::iterator;
   
   
   


private:
   
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1180; see docs/PROVENANCE.md.
namespace llvm {
template <class IntrusiveListT, class TraitsT>
class iplist_impl : public TraitsT, IntrusiveListT {
  using base_list_type = IntrusiveListT;

public:
  iplist_impl() = default;
  using iterator = typename IntrusiveListT::iterator;
  iterator begin();
  iterator end();
  iplist_impl(const iplist_impl &) = delete;
  iplist_impl &operator=(const iplist_impl &) = delete;
  inline iplist_impl(iplist_impl &&X);
  inline iplist_impl &operator=(iplist_impl &&X);
  inline ~iplist_impl();
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 1191; see docs/PROVENANCE.md.
namespace llvm {
template <class T, class... Options>
class iplist
    : public iplist_impl<simple_ilist<T, Options...>, ilist_traits<T>> {
  using iplist_impl_type = typename iplist::iplist_impl;

public:
  iplist() = default;
  iplist(const iplist &X) = delete;
  iplist &operator=(const iplist &X) = delete;
  inline iplist(iplist &&X);
  inline iplist &operator=(iplist &&X);
};
} // namespace llvm

#endif
