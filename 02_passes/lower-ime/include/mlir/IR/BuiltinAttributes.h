// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_IR_BUILTINATTRIBUTES_H
#define BUDDY_MINI_MLIR_IR_BUILTINATTRIBUTES_H
#include "mlir/IR/BuiltinTypes.h"

namespace mlir {
namespace detail {
 





 
 





} // namespace detail
 






namespace detail {
 



struct FloatAttrStorage;
struct IntegerAttrStorage;
} // namespace detail
// Explicit source instantiations make the retained bases visible to
// Frama-Clang.
class FloatAttr;
class IntegerAttr;
 


 


 





 







 






namespace detail {
 
 



template<typename Iterator> class ElementsAttrRange : public llvm::iterator_range<Iterator> {
public: using llvm::iterator_range<Iterator>::iterator_range;
  
 size_t size() const;
private: ShapedType shapeType;
};
}
 










} // namespace mlir
namespace mlir {
namespace detail { struct StridedLayoutAttrStorage; }
 






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
