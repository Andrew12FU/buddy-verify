// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_POINTERINTPAIR_H
#define BUDDY_MINI_LLVM_ADT_POINTERINTPAIR_H

#include "mini/Support/Standard.h"

// Provenance: audited Reshape V2 mini, line 299; see docs/PROVENANCE.md.
namespace llvm {
template <typename T> struct PointerLikeTypeTraits;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 300; see docs/PROVENANCE.md.
namespace llvm {
template <typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 302; see docs/PROVENANCE.md.
namespace llvm {
namespace detail {
template <typename Ptr> struct PunnedPointer {
  static_assert(sizeof(Ptr) == sizeof(long), "");
  static_assert(std::is_trivially_destructible<Ptr>::value, "");
  static_assert(std::is_trivially_copy_constructible<Ptr>::value, "");
  static_assert(std::is_trivially_move_constructible<Ptr>::value, "");
  explicit constexpr PunnedPointer(long i = 0);

private:
  alignas(Ptr) unsigned char Data[sizeof(Ptr)];
};
} // namespace detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 312; see docs/PROVENANCE.md.
namespace llvm {
template <typename PointerTy, unsigned IntBits, typename IntType = unsigned,
          typename PtrTraits = PointerLikeTypeTraits<PointerTy>,
          typename Info = PointerIntPairInfo<PointerTy, IntBits, PtrTraits>>
class PointerIntPair {
  using InfoTy = Info;
  detail::PunnedPointer<PointerTy> Value;

public:
  constexpr PointerIntPair() = default;
  PointerIntPair(PointerTy PtrVal, IntType IntVal);
  explicit PointerIntPair(PointerTy PtrVal);
};
} // namespace llvm

#endif
