// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_SUPPORT_CASTING_H
#define BUDDY_MINI_LLVM_SUPPORT_CASTING_H

#include "mlir/Support/Forward.h"
#include "llvm/ADT/PointerIntPair.h"

// Provenance: audited Reshape V2 mini, line 325; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From, typename Enable = void> struct CastInfo;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 326; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From, typename Enable = void>
struct CastIsPossible;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 327; see docs/PROVENANCE.md.
namespace llvm {
template <typename To> struct NullableValueCastFailed {
  static To castFailed();
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 328; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From, typename Derived>
struct DefaultDoCastIfPossible {
  static To doCastIfPossible(From f);
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 331; see docs/PROVENANCE.md.
namespace llvm {
namespace detail {
template <typename OptionalDerived, typename Default>
using SelfType = std::conditional_t<std::is_same<OptionalDerived, void>::value,
                                    Default, OptionalDerived>;
} // namespace detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 333; see docs/PROVENANCE.md.
namespace llvm {
namespace detail {
template <typename T> inline bool isPresent(const T &t);
} // namespace detail
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 335; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From, typename Derived = void>
struct ValueFromPointerCast
    : public CastIsPossible<To, From *>,
      public NullableValueCastFailed<To>,
      public DefaultDoCastIfPossible<
          To, From *,
          detail::SelfType<Derived, ValueFromPointerCast<To, From>>> {
  static inline To doCast(From *f);
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 342; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From, typename Enable> struct CastIsPossible {
  static bool isPossible(const From &);
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 345; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From>
struct CastInfo<
    To, From,
    std::enable_if_t<std::is_same<::mlir::Type, typename std::remove_const<
                                                    From>::type>::value ||
                     std::is_base_of<::mlir::Type, From>::value>>
    : NullableValueCastFailed<To>,
      DefaultDoCastIfPossible<To, From, CastInfo<To, From>> {
  static inline bool isPossible(::mlir::Type ty);
  static inline To doCast(::mlir::Type ty);
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 353; see docs/PROVENANCE.md.
namespace llvm {
template <typename T>
struct CastInfo<T, ::mlir::Operation *>
    : public ValueFromPointerCast<T, ::mlir::Operation,
                                  CastInfo<T, ::mlir::Operation *>> {
  static bool isPossible(::mlir::Operation *op);
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 357; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From>
[[nodiscard]] inline auto dyn_cast(const From &Val)
    -> decltype(CastInfo<To, const From>::doCastIfPossible(Val)) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) &&
                   "dyn_cast on a non-existent value");
  return CastInfo<To, const From>::doCastIfPossible(Val);
}
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 362; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From>
[[nodiscard]] inline auto
dyn_cast(From &Val) -> decltype(CastInfo<To, From>::doCastIfPossible(Val)) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) &&
                   "dyn_cast on a non-existent value");
  return CastInfo<To, From>::doCastIfPossible(Val);
}
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 367; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From>
[[nodiscard]] inline decltype(auto) dyn_cast(From *Val) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) &&
                   "dyn_cast on a non-existent value");
  return CastInfo<To, From *>::doCastIfPossible(Val);
}
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 372; see docs/PROVENANCE.md.
namespace llvm {
template <typename To, typename From>
[[nodiscard]] inline decltype(auto) dyn_cast(std::unique_ptr<From> &Val) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) &&
                   "dyn_cast on a non-existent value");
  return CastInfo<To, std::unique_ptr<From>>::doCastIfPossible(Val);
}
} // namespace llvm

namespace llvm {
// Same selected return identity as Casting.h; algorithm remains external.
template <typename To, typename From>
auto cast(const From &Val) -> decltype(CastInfo<To, const From>::doCast(Val));
} // namespace llvm
namespace llvm {
template<typename To, typename From>
struct CastInfo<To, From, std::enable_if_t<std::is_same<::mlir::Attribute, typename std::remove_const<From>::type>::value || std::is_base_of<::mlir::Attribute, From>::value>> : NullableValueCastFailed<To>, DefaultDoCastIfPossible<To, From, CastInfo<To, From>> {
  static bool isPossible(::mlir::Attribute attr);
  static To doCast(::mlir::Attribute attr);
};
template<typename To, typename From> bool isa(const From &Val);
}

namespace llvm {
template<typename To, typename From>
struct CastInfo<To, From, std::enable_if_t<std::is_same<::mlir::Value, typename std::remove_const<From>::type>::value || std::is_base_of<::mlir::Value, From>::value>> : NullableValueCastFailed<To>, DefaultDoCastIfPossible<To, From, CastInfo<To, From>> {
  static bool isPossible(::mlir::Value value);
  static To doCast(::mlir::Value value);
};
}

namespace llvm { template<typename To,typename From> auto dyn_cast_or_null(From *value) -> decltype(CastInfo<To,From *>::doCastIfPossible(value)); }
#endif
