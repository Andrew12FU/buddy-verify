// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_IR_BUILTINATTRIBUTES_H
#define BUDDY_MINI_MLIR_IR_BUILTINATTRIBUTES_H
#include "mlir/IR/BuiltinTypes.h"

namespace mlir {
namespace detail {
struct TypedAttrInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
template <typename T> struct TypedAttrTrait;
struct ElementsAttrInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
// Converter compatibility: exact specialization of the retained template.
// See header-rewrite.json for source and parameter substitutions.
class TypedAttr;
namespace detail {
template <>
class Interface<::mlir::TypedAttr, ::mlir::Attribute, ::mlir::detail::TypedAttrInterfaceTraits, ::mlir::Attribute, ::mlir::AttributeTrait::TraitBase> : public ::mlir::Attribute {
public:
  using Concept = typename ::mlir::detail::TypedAttrInterfaceTraits::Concept;
  template <typename T> using Model = typename ::mlir::detail::TypedAttrInterfaceTraits::template Model<T>;
  template <typename T>
  using FallbackModel = typename ::mlir::detail::TypedAttrInterfaceTraits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename ::mlir::detail::TypedAttrInterfaceTraits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<::mlir::TypedAttr, ::mlir::Attribute, ::mlir::detail::TypedAttrInterfaceTraits, ::mlir::Attribute, ::mlir::AttributeTrait::TraitBase>;
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
} // namespace detail
template class AttributeInterface<TypedAttr, detail::TypedAttrInterfaceTraits>;
class TypedAttr
    : public AttributeInterface<TypedAttr, detail::TypedAttrInterfaceTraits> {
public:
  using AttributeInterface<
      TypedAttr, detail::TypedAttrInterfaceTraits>::AttributeInterface;
  // Conversion view: preserve the inherited availability constraint.
  TypedAttr() = default;
  template <typename T,
            std::enable_if_t<std::is_base_of<InterfaceBase::template Trait<T>, T>::value> * = nullptr>
  TypedAttr(T t);

  template <typename T> struct Trait : detail::TypedAttrTrait<T> {};
};
namespace detail {
template <typename T>
struct TypedAttrTrait
    : AttributeInterface<TypedAttr,
                         TypedAttrInterfaceTraits>::template Trait<T> {};
struct FloatAttrStorage;
struct IntegerAttrStorage;
} // namespace detail
// Explicit source instantiations make the retained bases visible to
// Frama-Clang.
class FloatAttr;
class IntegerAttr;
 


template class detail::StorageUserBase<
    IntegerAttr, Attribute, detail::IntegerAttrStorage,
    detail::AttributeUniquer, TypedAttr::Trait>;
 





class IntegerAttr
    : public Attribute::AttrBase<IntegerAttr, Attribute,
                                 detail::IntegerAttrStorage, TypedAttr::Trait> {
public:
   
  using Base::Base;
   
};
class ElementsAttr;
namespace detail {
template <>
class Interface<::mlir::ElementsAttr, ::mlir::Attribute, ::mlir::detail::ElementsAttrInterfaceTraits, ::mlir::Attribute, ::mlir::AttributeTrait::TraitBase> : public ::mlir::Attribute {
public:
  using Concept = typename ::mlir::detail::ElementsAttrInterfaceTraits::Concept;
  template <typename T> using Model = typename ::mlir::detail::ElementsAttrInterfaceTraits::template Model<T>;
  template <typename T>
  using FallbackModel = typename ::mlir::detail::ElementsAttrInterfaceTraits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename ::mlir::detail::ElementsAttrInterfaceTraits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<::mlir::ElementsAttr, ::mlir::Attribute, ::mlir::detail::ElementsAttrInterfaceTraits, ::mlir::Attribute, ::mlir::AttributeTrait::TraitBase>;
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
template class AttributeInterface<ElementsAttr, detail::ElementsAttrInterfaceTraits>;
class ElementsAttr : public AttributeInterface<ElementsAttr,
                                detail::ElementsAttrInterfaceTraits> {
public:
  using AttributeInterface<
      ElementsAttr, detail::ElementsAttrInterfaceTraits>::AttributeInterface;
  template<typename T> struct Trait : AttributeInterface<ElementsAttr,detail::ElementsAttrInterfaceTraits>::template Trait<T> {};
};
namespace detail {
using DenseIterPtrAndSplat = std::pair<const char *,bool>;
template<typename ConcreteT,typename T,typename PointerT=T *,typename ReferenceT=T &>
class DenseElementIndexedIteratorImpl : public llvm::indexed_accessor_iterator<ConcreteT,DenseIterPtrAndSplat,T,PointerT,ReferenceT> {
protected: DenseElementIndexedIteratorImpl(const char *data,bool isSplat,size_t dataIndex);
};
template<typename Iterator> class ElementsAttrRange : public llvm::iterator_range<Iterator> {
public: using llvm::iterator_range<Iterator>::iterator_range;
  
 size_t size() const;
private: ShapedType shapeType;
};
}
class DenseElementsAttr : public Attribute {
public:
  using Attribute::Attribute;
  template<typename T> class ElementIterator : public detail::DenseElementIndexedIteratorImpl<ElementIterator<T>,const T> {
  public: const T &operator*() const;
  private: ElementIterator(const char *data,bool isSplat,size_t dataIndex);
  };
   
  operator ElementsAttr() const;
   
};
} // namespace mlir
namespace mlir {
namespace detail { struct StridedLayoutAttrStorage; }
class StridedLayoutAttr : public Attribute::AttrBase<StridedLayoutAttr, Attribute, detail::StridedLayoutAttrStorage, MemRefLayoutAttrInterface::Trait> {
public:
  using Base::Base;
  static StridedLayoutAttr get(MLIRContext *context, int64_t offset, ArrayRef<int64_t> strides);
  int64_t getOffset() const;
  ArrayRef<int64_t> getStrides() const;
};
}

namespace mlir {
namespace detail { struct ArrayAttrStorage; }
 
namespace detail { struct DenseTypedElementsAttrStorage; }
 
 
}

#include "mlir/IR/AffineMap.h"
namespace mlir {
namespace detail { struct StringAttrStorage; struct AffineMapAttrStorage; struct DenseArrayAttrStorage; }
 
// NamedAttribute retains its actual pair of a name and an attribute handle.
 



 


 
namespace detail {
 




}
 
 
}

#endif
