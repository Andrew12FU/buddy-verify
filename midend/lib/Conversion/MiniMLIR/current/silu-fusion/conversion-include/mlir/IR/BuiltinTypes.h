// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_BUILTINTYPES_H
#define BUDDY_MINI_MLIR_IR_BUILTINTYPES_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Types.h"

// Provenance: audited Reshape V2 mini, line 963; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct ShapedTypeInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 969; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteType> struct ShapedTypeTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 971; see docs/PROVENANCE.md.
namespace mlir {
class ShapedType
    : public TypeInterface<ShapedType, detail::ShapedTypeInterfaceTraits> {
public:
  using TypeInterface<ShapedType,
                      detail::ShapedTypeInterfaceTraits>::TypeInterface;
  template <typename ConcreteType>
  struct Trait : public detail::ShapedTypeTrait<ConcreteType> {};
   
   
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 977; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteType>
struct ShapedTypeTrait
    : public TypeInterface<ShapedType, detail::ShapedTypeInterfaceTraits>::
          template Trait<ConcreteType> {
   
   
  inline size_t getNumDynamicDims() const;
  int64_t getRank() const;
   
   
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 982; see docs/PROVENANCE.md.
namespace mlir {
class TensorType;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 983; see docs/PROVENANCE.md.
namespace mlir {
 
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 985; see docs/PROVENANCE.md.
namespace mlir {
class TensorType : public Type, public ShapedType::Trait<TensorType> {
public:
  using Type::Type;
  inline operator ShapedType() const;
   
   
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 992; see docs/PROVENANCE.md.
namespace mlir {
template <typename ConcreteType>
class ValueSemantics
    : public TypeTrait::TraitBase<ConcreteType, ValueSemantics> {};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 995; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct RankedTensorTypeStorage;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 996; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteT, typename BaseT, typename StorageT,
          typename UniquerT, template <typename> class... Traits>
class StorageUserBase : public BaseT, public Traits<ConcreteT>... {
public:
  using BaseT::BaseT;
  using Base = StorageUserBase<ConcreteT, BaseT, StorageT, UniquerT, Traits...>;
  using ImplType = StorageT;
  using HasTraitFn = bool (*)(TypeID);
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1007; see docs/PROVENANCE.md.
namespace mlir {
class RankedTensorType
    : public Type::TypeBase<RankedTensorType, TensorType,
                            detail::RankedTensorTypeStorage, ShapedType::Trait,
                            ValueSemantics> {
public:
  using Base::Base;
  using ShapedType::Trait<RankedTensorType>::getNumDynamicDims;
  using ShapedType::Trait<RankedTensorType>::getRank;
  ArrayRef<int64_t> getShape() const;
  Type getElementType() const;
   

};
} // namespace mlir

namespace mlir {
namespace detail {
struct IntegerTypeStorage;
}
// IntegerType's unused interface mixins are outside this bounded member
// surface.
 







} // namespace mlir
namespace mlir { namespace detail {
 
 
}
 




namespace detail {   }
}

namespace mlir {
namespace detail { struct MemRefTypeStorage; }
 








class MemRefLayoutAttrInterface;
class AffineMap;
 











}

namespace mlir { namespace detail {
 
 
}
 




namespace detail {   }
}

namespace mlir { namespace detail {
 
 
}
 




namespace detail {   }
}

namespace mlir {
 
}


namespace mlir {
namespace detail { struct FunctionTypeStorage; }
 



}

namespace mlir {
namespace detail { struct VectorTypeStorage; struct IntegerTypeStorage; }
 



namespace detail {   }
 



namespace detail {   }

}
#endif
