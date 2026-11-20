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
  bool isDynamicDim(unsigned index) const;
  int64_t getDimSize(unsigned index) const;
   
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
 






} // namespace mlir
// Provenance: audited Reshape V2 mini, line 992; see docs/PROVENANCE.md.
namespace mlir {
 


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
 












} // namespace mlir

namespace mlir {
namespace detail {
struct IntegerTypeStorage;
}
// IntegerType's unused interface mixins are outside this bounded member
// surface.
 







} // namespace mlir
namespace mlir { namespace detail {
struct PtrLikeTypeInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct PtrLikeTypeInterfaceTrait;
}
class PtrLikeTypeInterface : public TypeInterface<PtrLikeTypeInterface, detail::PtrLikeTypeInterfaceInterfaceTraits> {
public:
  using TypeInterface<PtrLikeTypeInterface, detail::PtrLikeTypeInterfaceInterfaceTraits>::TypeInterface;
  template<typename T> struct Trait : detail::PtrLikeTypeInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct PtrLikeTypeInterfaceTrait : TypeInterface<PtrLikeTypeInterface, PtrLikeTypeInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir {
namespace detail { struct MemRefTypeStorage; }
class BaseMemRefType : public Type, public PtrLikeTypeInterface::Trait<BaseMemRefType>, public ShapedType::Trait<BaseMemRefType> {
public:
  using Type::Type;
   
   
   
  Attribute getMemorySpace() const;
  operator ShapedType() const;
};
class MemRefLayoutAttrInterface;
class AffineMap;
class MemRefType : public Type::TypeBase<MemRefType, BaseMemRefType, detail::MemRefTypeStorage, PtrLikeTypeInterface::Trait, ShapedType::Trait> {
public:
  using Base::Base;
  using ShapedType::Trait<MemRefType>::getRank;
  using ShapedType::Trait<MemRefType>::isDynamicDim;
  using ShapedType::Trait<MemRefType>::getDimSize;
  static MemRefType get(ArrayRef<int64_t> shape, Type elementType, MemRefLayoutAttrInterface layout = {}, Attribute memorySpace = {});
  static MemRefType get(ArrayRef<int64_t> shape,Type elementType,AffineMap map,Attribute memorySpace = {});
   
  ArrayRef<int64_t> getShape() const;
  Type getElementType() const;
};
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
class FunctionType : public Type::TypeBase<FunctionType,Type,detail::FunctionTypeStorage> {
public: using Base::Base;
   ArrayRef<Type> getResults() const;
};
}

namespace mlir {
namespace detail { struct VectorTypeStorage; struct IntegerTypeStorage; }
 



namespace detail {   }
 



namespace detail {   }

}
#endif
