// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_ILIST_NODE_H
#define BUDDY_MINI_LLVM_ADT_ILIST_NODE_H

#include "llvm/ADT/PointerIntPair.h"

#define LLVM_ENABLE_ABI_BREAKING_CHECKS 1
// Provenance: audited Reshape V2 mini, line 381; see docs/PROVENANCE.md.
namespace llvm {
template <bool EnableSentinelTracking, class ParentTy> class ilist_node_base;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 382; see docs/PROVENANCE.md.
namespace llvm {
template <bool EnableSentinelTracking, class ParentTy> class ilist_base;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 389; see docs/PROVENANCE.md.
namespace llvm {
template <bool EnableSentinelTracking> struct ilist_sentinel_tracking {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 395; see docs/PROVENANCE.md.
namespace llvm {
template <class Tag> struct ilist_tag {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 403; see docs/PROVENANCE.md.
namespace llvm {
template <bool ExtraIteratorBits> struct ilist_iterator_bits {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 416; see docs/PROVENANCE.md.
namespace llvm {
template <class ParentTy> struct ilist_parent {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 421; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <bool IsExplicit> struct explicitness {
  static const bool is_explicit = IsExplicit;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 424; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
using is_explicit = explicitness<true>;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 425; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
using is_implicit = explicitness<false>;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 438; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Option> struct is_valid_option : std::false_type {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 444; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class... Options> struct extract_sentinel_tracking;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 445; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <bool EnableSentinelTracking, class... Options>
struct extract_sentinel_tracking<
    ilist_sentinel_tracking<EnableSentinelTracking>, Options...>
    : std::bool_constant<EnableSentinelTracking>, is_explicit {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 449; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Option1, class... Options>
struct extract_sentinel_tracking<Option1, Options...>
    : extract_sentinel_tracking<Options...> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 453; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <> struct extract_sentinel_tracking<> : std::true_type, is_implicit {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 458; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <bool EnableSentinelTracking>
struct is_valid_option<ilist_sentinel_tracking<EnableSentinelTracking>>
    : std::true_type {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 466; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class... Options> struct extract_tag;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 467; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Tag, class... Options>
struct extract_tag<ilist_tag<Tag>, Options...> {
  using type = Tag;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 471; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Option1, class... Options>
struct extract_tag<Option1, Options...> : extract_tag<Options...> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 473; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <> struct extract_tag<> {
  using type = void;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 476; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Tag> struct is_valid_option<ilist_tag<Tag>> : std::true_type {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 482; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class... Options> struct extract_iterator_bits;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 483; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <bool IteratorBits, class... Options>
struct extract_iterator_bits<ilist_iterator_bits<IteratorBits>, Options...>
    : std::bool_constant<IteratorBits> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 486; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Option1, class... Options>
struct extract_iterator_bits<Option1, Options...>
    : extract_iterator_bits<Options...> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 489; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <> struct extract_iterator_bits<> : std::false_type, is_implicit {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 490; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <bool IteratorBits>
struct is_valid_option<ilist_iterator_bits<IteratorBits>> : std::true_type {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 497; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class... Options> struct extract_parent;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 498; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class ParentTy, class... Options>
struct extract_parent<ilist_parent<ParentTy>, Options...> {
  using type = ParentTy;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 502; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class Option1, class... Options>
struct extract_parent<Option1, Options...> : extract_parent<Options...> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 504; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <> struct extract_parent<> {
  using type = void;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 507; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class ParentTy>
struct is_valid_option<ilist_parent<ParentTy>> : std::true_type {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 513; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class... Options>
struct check_options : std::conjunction<is_valid_option<Options>...> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 519; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class T, bool EnableSentinelTracking, bool IsSentinelTrackingExplicit,
          class TagT, bool HasIteratorBits, class ParentTy>
struct node_options {
  using value_type = T;
  using pointer = T *;
  using reference = T &;
  using const_pointer = const T *;
  using const_reference = const T &;

  static const bool enable_sentinel_tracking = EnableSentinelTracking;
  static const bool is_sentinel_tracking_explicit = IsSentinelTrackingExplicit;
  static const bool has_iterator_bits = HasIteratorBits;
  using tag = TagT;
  using parent_ty = ParentTy;
  using node_base_type = ilist_node_base<enable_sentinel_tracking, parent_ty>;
  using list_base_type = ilist_base<enable_sentinel_tracking, parent_ty>;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 537; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class T, class... Options> struct compute_node_options {
  using type = node_options<T, extract_sentinel_tracking<Options...>::value,
                            extract_sentinel_tracking<Options...>::is_explicit,
                            typename extract_tag<Options...>::type,
                            extract_iterator_bits<Options...>::value,
                            typename extract_parent<Options...>::type>;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 547; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class NodeBase, bool Enable> class node_base_prevnext;
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 548; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class NodeBase> class node_base_prevnext<NodeBase, false> {
  NodeBase *Prev = nullptr;
  NodeBase *Next = nullptr;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 552; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class NodeBase> class node_base_prevnext<NodeBase, true> {
  PointerIntPair<NodeBase *, 1> PrevAndSentinel;
  NodeBase *Next = nullptr;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 556; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class ParentTy> class node_base_parent {
  ParentTy *Parent = nullptr;
};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 557; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <> class node_base_parent<void> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 558; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class NodeTy, class ParentTy> class node_parent_access {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 559; see docs/PROVENANCE.md.
namespace llvm {
namespace ilist_detail {
template <class NodeTy> class node_parent_access<NodeTy, void> {};
} // namespace ilist_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 561; see docs/PROVENANCE.md.
namespace llvm {
template <bool EnableSentinelTracking, class ParentTy>
class ilist_node_base : public ilist_detail::node_base_prevnext<
                            ilist_node_base<EnableSentinelTracking, ParentTy>,
                            EnableSentinelTracking>,
                        public ilist_detail::node_base_parent<ParentTy> {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 565; see docs/PROVENANCE.md.
namespace llvm {
template <class OptionsT>
class ilist_node_impl
    : OptionsT::node_base_type,
      public ilist_detail::node_parent_access<ilist_node_impl<OptionsT>,
                                              typename OptionsT::parent_ty> {
protected:
  ilist_node_impl() = default;
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 571; see docs/PROVENANCE.md.
namespace llvm {
template <class T, class... Options>
class ilist_node
    : public ilist_node_impl<
          typename ilist_detail::compute_node_options<T, Options...>::type> {
  static_assert(ilist_detail::check_options<Options...>::value,
                "Unrecognized node option!");
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 575; see docs/PROVENANCE.md.
namespace llvm {
template <typename NodeTy, typename ParentTy, class... Options>
class ilist_node_with_parent : public ilist_node<NodeTy, Options...> {
protected:
  ilist_node_with_parent() = default;
};
} // namespace llvm

#endif
