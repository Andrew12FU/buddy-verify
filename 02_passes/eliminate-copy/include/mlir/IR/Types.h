// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_TYPES_H
#define BUDDY_MINI_MLIR_IR_TYPES_H

#include "mlir/Support/InterfaceSupport.h"

// Provenance: audited Reshape V2 mini, line 918; see docs/PROVENANCE.md.
namespace mlir {
class TypeStorage;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 919; see docs/PROVENANCE.md.
namespace mlir {
class TypeID;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 921; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
class TypeUniquer;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 922; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteT, typename BaseT, typename StorageT,
          typename UniquerT, template <typename> class... Traits>
class StorageUserBase;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 925; see docs/PROVENANCE.md.
namespace mlir {
class Type {
public:
  template <typename ConcreteType, typename BaseType, typename StorageType,
            template <typename> class... Traits>
  using TypeBase = detail::StorageUserBase<ConcreteType, BaseType, StorageType,
                                           detail::TypeUniquer, Traits...>;
  using ImplType = TypeStorage;
  constexpr Type() = default;
  inline Type(const ImplType *impl);
  Type(const Type &other) = default;
  Type &operator=(const Type &other) = default;
  inline explicit operator bool() const;
  inline bool operator!() const;
  inline bool operator!=(Type other) const;
  MLIRContext *getContext() const;

protected:
  ImplType *impl{nullptr};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 944; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteType, template <typename> class TraitType>
class StorageUserTraitBase {
protected:
   
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 951; see docs/PROVENANCE.md.
namespace mlir {
namespace TypeTrait {
template <typename ConcreteType, template <typename> class TraitType>
using TraitBase = detail::StorageUserTraitBase<ConcreteType, TraitType>;
} // namespace TypeTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 954; see docs/PROVENANCE.md.
namespace mlir {
template <typename ConcreteType, typename Traits>
class TypeInterface : public detail::Interface<ConcreteType, Type, Traits, Type,
                                               TypeTrait::TraitBase> {
public:
  using Base = TypeInterface<ConcreteType, Traits>;
  using InterfaceBase =
      detail::Interface<ConcreteType, Type, Traits, Type, TypeTrait::TraitBase>;
  using InterfaceBase::InterfaceBase;
};
} // namespace mlir

#endif
