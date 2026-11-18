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
 
 
}
 




namespace detail {   }
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
