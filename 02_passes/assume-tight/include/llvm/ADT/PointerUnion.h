// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_POINTERUNION_H
#define BUDDY_MINI_LLVM_ADT_POINTERUNION_H

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/STLExtras.h"

// Provenance: audited Reshape V2 mini, line 620; see docs/PROVENANCE.md.
namespace llvm {
namespace pointer_union_detail {
template <typename Derived, int Idx, typename... Types>
class PointerUnionMembers;
} // namespace pointer_union_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 621; see docs/PROVENANCE.md.
namespace llvm {
namespace pointer_union_detail {
template <typename Derived, int Idx> class PointerUnionMembers<Derived, Idx> {
protected:
  detail::PunnedPointer<void *> Val;
  PointerUnionMembers();
};
} // namespace pointer_union_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 626; see docs/PROVENANCE.md.
namespace llvm {
namespace pointer_union_detail {
template <typename Derived, int Idx, typename Type, typename... Types>
class PointerUnionMembers<Derived, Idx, Type, Types...>
    : public PointerUnionMembers<Derived, Idx + 1, Types...> {
  using Base = PointerUnionMembers<Derived, Idx + 1, Types...>;

public:
  using Base::Base;
  PointerUnionMembers() = default;
  PointerUnionMembers(Type V);
  using Base::operator=;
  Derived &operator=(Type V);
};
} // namespace pointer_union_detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 638; see docs/PROVENANCE.md.
namespace llvm {
template <typename... PTs>
class PointerUnion
    : public pointer_union_detail::PointerUnionMembers<PointerUnion<PTs...>, 0,
                                                       PTs...> {
  static_assert(sizeof...(PTs) > 0, "PointerUnion must have at least one type");
  static_assert(TypesAreDistinct<PTs...>::value,
                "PointerUnion alternative types cannot be repeated");
  using Base = pointer_union_detail::PointerUnionMembers<PointerUnion<PTs...>,
                                                         0, PTs...>;

public:
  PointerUnion() = default;
  PointerUnion(std::nullptr_t);
  using Base::Base;
  using Base::operator=;
  const PointerUnion &operator=(std::nullptr_t);
  // Tag-table computations are not used at this declaration boundary.
};
} // namespace llvm

#endif
