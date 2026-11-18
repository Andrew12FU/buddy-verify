// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_SUPPORT_TRAILINGOBJECTS_H
#define BUDDY_MINI_LLVM_SUPPORT_TRAILINGOBJECTS_H

#include "mini/Support/Standard.h"

// Provenance: audited Reshape V2 mini, line 583; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
template <typename... Ts> struct MaxAlignmentDecl;
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 584; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
template <typename T> struct MaxAlignmentDecl<T> {
  static constexpr size_t value = alignof(T);
};
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 585; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
template <typename T, typename U, typename... Ts>
struct MaxAlignmentDecl<T, U, Ts...> {
  static constexpr size_t tail = MaxAlignmentDecl<U, Ts...>::value;
  static constexpr size_t value = alignof(T) > tail ? alignof(T) : tail;
};
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 589; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
class TrailingObjectsBase {
protected:
  template <typename T> struct OverloadToken {};
};
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 593; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
template <int Align, typename BaseTy, typename TopTrailingObj, typename PrevTy,
          typename... MoreTys>
class TrailingObjectsImpl {};
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 595; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
template <int Align, typename BaseTy, typename TopTrailingObj, typename PrevTy,
          typename NextTy, typename... MoreTys>
class TrailingObjectsImpl<Align, BaseTy, TopTrailingObj, PrevTy, NextTy,
                          MoreTys...>
    : public TrailingObjectsImpl<Align, BaseTy, TopTrailingObj, NextTy,
                                 MoreTys...> {};
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 598; see docs/PROVENANCE.md.
namespace llvm {
namespace trailing_objects_internal {
template <int Align, typename BaseTy, typename TopTrailingObj, typename PrevTy>
class alignas(Align) TrailingObjectsImpl<Align, BaseTy, TopTrailingObj, PrevTy>
    : public TrailingObjectsBase {};
} // namespace trailing_objects_internal
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 601; see docs/PROVENANCE.md.
namespace llvm {
template <typename BaseTy, typename... TrailingTys>
class TrailingObjects
    : private trailing_objects_internal::TrailingObjectsImpl<
          trailing_objects_internal::MaxAlignmentDecl<TrailingTys...>::value,
          BaseTy, TrailingObjects<BaseTy, TrailingTys...>, BaseTy,
          TrailingTys...> {
public:
  TrailingObjects() = default;
  TrailingObjects(const TrailingObjects &) = delete;
  TrailingObjects(TrailingObjects &&) = delete;
  TrailingObjects &operator=(const TrailingObjects &) = delete;
  TrailingObjects &operator=(TrailingObjects &&) = delete;
};
} // namespace llvm

#endif
