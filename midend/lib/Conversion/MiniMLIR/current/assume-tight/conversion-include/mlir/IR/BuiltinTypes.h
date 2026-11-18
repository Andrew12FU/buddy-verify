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
   
  static constexpr bool isDynamic(int64_t dValue);
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
class PtrLikeTypeInterface;
namespace detail {
template <>
class Interface<::mlir::PtrLikeTypeInterface, ::mlir::Type, ::mlir::detail::PtrLikeTypeInterfaceInterfaceTraits, ::mlir::Type, ::mlir::TypeTrait::TraitBase> : public ::mlir::Type {
public:
  using Concept = typename ::mlir::detail::PtrLikeTypeInterfaceInterfaceTraits::Concept;
  template <typename T> using Model = typename ::mlir::detail::PtrLikeTypeInterfaceInterfaceTraits::template Model<T>;
  template <typename T>
  using FallbackModel = typename ::mlir::detail::PtrLikeTypeInterfaceInterfaceTraits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename ::mlir::detail::PtrLikeTypeInterfaceInterfaceTraits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<::mlir::PtrLikeTypeInterface, ::mlir::Type, ::mlir::detail::PtrLikeTypeInterfaceInterfaceTraits, ::mlir::Type, ::mlir::TypeTrait::TraitBase>;
  using ValueType = ::mlir::Type;
  template <typename ConcreteT>
  struct Trait : public ::mlir::TypeTrait::TraitBase<ConcreteT, Trait> {
    using ModelT = Model<ConcreteT>;
  };
  explicit Interface(::mlir::Type t = ::mlir::Type());
  Interface(std::nullptr_t);
  template <typename T,
            std::enable_if_t<std::is_base_of<Trait<T>, T>::value> * = nullptr>
  Interface(T t);
  Interface(::mlir::Type t, const Concept *conceptImpl);
  Interface(::mlir::Type t, std::nullptr_t);

private:
  Concept *conceptImpl;
};
}
template class TypeInterface<PtrLikeTypeInterface, detail::PtrLikeTypeInterfaceInterfaceTraits>;
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
   
  bool hasRank() const;
   
  Attribute getMemorySpace() const;
  operator ShapedType() const;
};
class MemRefLayoutAttrInterface;
class AffineMap;
class MemRefType;
template class detail::StorageUserBase<MemRefType, BaseMemRefType, detail::MemRefTypeStorage, detail::TypeUniquer, PtrLikeTypeInterface::Trait, ShapedType::Trait>;
class MemRefType : public Type::TypeBase<MemRefType, BaseMemRefType, detail::MemRefTypeStorage, PtrLikeTypeInterface::Trait, ShapedType::Trait> {
public:
  using Base::Base;
  using ShapedType::Trait<MemRefType>::getRank;
  using ShapedType::Trait<MemRefType>::isDynamicDim;
  using ShapedType::Trait<MemRefType>::getDimSize;
  static MemRefType get(ArrayRef<int64_t> shape, Type elementType, MemRefLayoutAttrInterface layout = {}, Attribute memorySpace = {});
  static MemRefType get(ArrayRef<int64_t> shape,Type elementType,AffineMap map,Attribute memorySpace = {});
  MemRefLayoutAttrInterface getLayout() const;
  ArrayRef<int64_t> getShape() const;
  Type getElementType() const;
};
}

namespace mlir { namespace detail {
struct DenseElementTypeInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct DenseElementTypeTrait;
}
class DenseElementType : public TypeInterface<DenseElementType, detail::DenseElementTypeInterfaceTraits> {
public:
  using TypeInterface<DenseElementType, detail::DenseElementTypeInterfaceTraits>::TypeInterface;
  template<typename T> struct Trait : detail::DenseElementTypeTrait<T> {};
};
namespace detail { template<typename T> struct DenseElementTypeTrait : TypeInterface<DenseElementType, DenseElementTypeInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace detail {
struct VectorElementTypeInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct VectorElementTypeInterfaceTrait;
}
class VectorElementTypeInterface : public TypeInterface<VectorElementTypeInterface, detail::VectorElementTypeInterfaceInterfaceTraits> {
public:
  using TypeInterface<VectorElementTypeInterface, detail::VectorElementTypeInterfaceInterfaceTraits>::TypeInterface;
  template<typename T> struct Trait : detail::VectorElementTypeInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct VectorElementTypeInterfaceTrait : TypeInterface<VectorElementTypeInterface, VectorElementTypeInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir {
class IndexType : public Type::TypeBase<IndexType, Type, TypeStorage, DenseElementType::Trait, VectorElementTypeInterface::Trait> { public: using Base::Base; };
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
