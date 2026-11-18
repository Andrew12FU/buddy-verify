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
class TypedAttr
    : public AttributeInterface<TypedAttr, detail::TypedAttrInterfaceTraits> {
public:
  using AttributeInterface<
      TypedAttr, detail::TypedAttrInterfaceTraits>::AttributeInterface;
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
 typename Iterator::reference operator[](uint64_t index) const;
 size_t size() const;
private: ShapedType shapeType;
};
}
class DenseElementsAttr : public Attribute {
public:
  using Attribute::Attribute;
  template<typename T> class ElementIterator : public detail::DenseElementIndexedIteratorImpl<ElementIterator<T>,const T> {
  public:  
  private: ElementIterator(const char *data,bool isSplat,size_t dataIndex);
  };
  template<typename T> detail::ElementsAttrRange<ElementIterator<T>> getValues() const;
  operator ElementsAttr() const;
   
};
} // namespace mlir
namespace mlir {
namespace detail { struct StridedLayoutAttrStorage; }
 






}

namespace mlir {
namespace detail { struct ArrayAttrStorage; }
class ArrayAttr : public Attribute::AttrBase<ArrayAttr,Attribute,detail::ArrayAttrStorage> { public: using Base::Base; };
namespace detail { struct DenseTypedElementsAttrStorage; }
class DenseTypedElementsAttr : public Attribute::AttrBase<DenseTypedElementsAttr,DenseElementsAttr,detail::DenseTypedElementsAttrStorage,TypedAttr::Trait,ElementsAttr::Trait> { public: using Base::Base; };
class DenseIntElementsAttr : public DenseTypedElementsAttr { public: using DenseTypedElementsAttr::DenseTypedElementsAttr; };
}

#include "mlir/IR/AffineMap.h"
namespace mlir {
namespace detail { struct StringAttrStorage; struct AffineMapAttrStorage; struct DenseArrayAttrStorage; }
 
// NamedAttribute retains its actual pair of a name and an attribute handle.
 



 


 
namespace detail {
 




}
 
 
}

#endif
