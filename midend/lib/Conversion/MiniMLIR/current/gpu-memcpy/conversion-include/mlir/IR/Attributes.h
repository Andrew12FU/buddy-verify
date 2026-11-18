// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_IR_ATTRIBUTES_H
#define BUDDY_MINI_MLIR_IR_ATTRIBUTES_H
#include "mlir/IR/Types.h"

namespace mlir {
class AttributeStorage;
namespace detail {
class AttributeUniquer;
}
class Attribute {
public:
  template <typename ConcreteType, typename BaseType, typename StorageType,
            template <typename> class... Traits>
  using AttrBase = detail::StorageUserBase<ConcreteType, BaseType, StorageType,
                                           detail::AttributeUniquer, Traits...>;
  constexpr Attribute() = default;
  Attribute(const AttributeStorage *impl);
  explicit operator bool() const;
   

protected:
  AttributeStorage *impl = nullptr;
};
namespace AttributeTrait {
template <typename ConcreteType, template <typename> class TraitType>
using TraitBase = detail::StorageUserTraitBase<ConcreteType, TraitType>;
}
template <typename ConcreteType, typename Traits>
class AttributeInterface
    : public detail::Interface<ConcreteType, Attribute, Traits, Attribute,
                               AttributeTrait::TraitBase> {
public:
  using InterfaceBase = detail::Interface<ConcreteType, Attribute, Traits,
                                          Attribute, AttributeTrait::TraitBase>;
  using InterfaceBase::InterfaceBase;
};
class NamedAttribute;
class StringAttr;
} // namespace mlir
namespace mlir { namespace detail {
struct MemRefLayoutAttrInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct MemRefLayoutAttrInterfaceTrait;
}
class MemRefLayoutAttrInterface;
namespace detail {
template <>
class Interface<::mlir::MemRefLayoutAttrInterface, ::mlir::Attribute, ::mlir::detail::MemRefLayoutAttrInterfaceInterfaceTraits, ::mlir::Attribute, ::mlir::AttributeTrait::TraitBase> : public ::mlir::Attribute {
public:
  using Concept = typename ::mlir::detail::MemRefLayoutAttrInterfaceInterfaceTraits::Concept;
  template <typename T> using Model = typename ::mlir::detail::MemRefLayoutAttrInterfaceInterfaceTraits::template Model<T>;
  template <typename T>
  using FallbackModel = typename ::mlir::detail::MemRefLayoutAttrInterfaceInterfaceTraits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename ::mlir::detail::MemRefLayoutAttrInterfaceInterfaceTraits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<::mlir::MemRefLayoutAttrInterface, ::mlir::Attribute, ::mlir::detail::MemRefLayoutAttrInterfaceInterfaceTraits, ::mlir::Attribute, ::mlir::AttributeTrait::TraitBase>;
  using ValueType = ::mlir::Attribute;
  template <typename ConcreteT>
  struct Trait : public ::mlir::AttributeTrait::TraitBase<ConcreteT, Trait> {
    using ModelT = Model<ConcreteT>;
  };
  explicit Interface(::mlir::Attribute t = ::mlir::Attribute());
  Interface(std::nullptr_t);
  template <typename T,
            std::enable_if_t<std::is_base_of<Trait<T>, T>::value> * = nullptr>
  Interface(T t);
  Interface(::mlir::Attribute t, const Concept *conceptImpl);
  Interface(::mlir::Attribute t, std::nullptr_t);

private:
  Concept *conceptImpl;
};
}
template class AttributeInterface<MemRefLayoutAttrInterface, detail::MemRefLayoutAttrInterfaceInterfaceTraits>;
class MemRefLayoutAttrInterface : public AttributeInterface<MemRefLayoutAttrInterface, detail::MemRefLayoutAttrInterfaceInterfaceTraits> {
public:
  using AttributeInterface<MemRefLayoutAttrInterface, detail::MemRefLayoutAttrInterfaceInterfaceTraits>::AttributeInterface;
  MemRefLayoutAttrInterface() = default;
  template<typename T, std::enable_if_t<std::is_base_of<InterfaceBase::template Trait<T>, T>::value> * = nullptr>
  MemRefLayoutAttrInterface(T t);

  template<typename T> struct Trait : detail::MemRefLayoutAttrInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct MemRefLayoutAttrInterfaceTrait : AttributeInterface<MemRefLayoutAttrInterface, MemRefLayoutAttrInterfaceInterfaceTraits>::template Trait<T> {}; }
}


namespace mlir { namespace detail {
 
 
}
 



namespace detail {   }
}


namespace mlir { namespace detail {
 
 
}
 



namespace detail {   }
}

#endif
