// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_SUPPORT_TYPEID_H
#define BUDDY_MINI_MLIR_SUPPORT_TYPEID_H

#include "mlir/Support/LLVM.h"
#include "llvm/ADT/STLExtras.h"

// Provenance: audited Reshape V2 mini, line 1725; see docs/PROVENANCE.md.
namespace mlir {
class TypeID {
  struct alignas(8) Storage {};

public:
  inline TypeID();
   
   
   
   
   
  inline static TypeID getFromOpaquePointer(const void *pointer);

private:
  inline TypeID(const Storage *storage);
  const Storage *storage;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1740; see docs/PROVENANCE.md.
namespace mlir {
class alignas(8) SelfOwningTypeID {
public:
  SelfOwningTypeID() = default;
  SelfOwningTypeID(const SelfOwningTypeID &) = delete;
  SelfOwningTypeID &operator=(const SelfOwningTypeID &) = delete;
  SelfOwningTypeID(SelfOwningTypeID &&) = delete;
  SelfOwningTypeID &operator=(SelfOwningTypeID &&) = delete;
  operator TypeID() const { return getTypeID(); }
  TypeID getTypeID() const { return TypeID::getFromOpaquePointer(this); }
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1751; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct InlineTypeIDResolver {
  template <typename T>
  using has_resolve_typeid_trait = decltype(T::resolveTypeID());
  template <typename T>
  using has_resolve_typeid = llvm::is_detected<has_resolve_typeid_trait, T>;
  template <typename T> static TypeID resolveTypeID() {
    return T::resolveTypeID();
  }
};
} // namespace detail
} // namespace mlir
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CLASS_NAME)               \
  static ::mlir::TypeID resolveTypeID() {                                      \
    static ::mlir::SelfOwningTypeID id;                                        \
    return id;                                                                 \
  }                                                                            \
  static_assert(                                                               \
      ::mlir::detail::InlineTypeIDResolver::has_resolve_typeid<                \
          CLASS_NAME>::value,                                                  \
      "`MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID` must be placed in a "    \
      "public section of `" #CLASS_NAME "`");

#endif
