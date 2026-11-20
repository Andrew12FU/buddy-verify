// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_IR_TYPERANGE_H
#define BUDDY_MINI_MLIR_IR_TYPERANGE_H
#include "mlir/IR/ValueRange.h"
namespace mlir {
class TypeRange : public llvm::detail::indexed_accessor_range_base<TypeRange,llvm::PointerUnion<const Value *,const Type *,OpOperand *,detail::OpResultImpl *,const llvm::Repeated<Type> *,const llvm::Repeated<Value> *>,Type,Type,Type> {
public:
  using RangeBaseT::RangeBaseT;
  template<typename ValueRangeT> TypeRange(ValueTypeRange<ValueRangeT> values);
 TypeRange(ArrayRef<Type> types = {});
 template<typename Arg,typename=std::enable_if_t<std::is_constructible<ArrayRef<Type>,Arg>::value>> TypeRange(Arg &&arg [[clang::lifetimebound]]);
 TypeRange(std::initializer_list<Type> types [[clang::lifetimebound]]);
};
}


#include "llvm/ADT/iterator_range.h"
namespace mlir {
template<typename ValueIteratorT>
class ValueTypeIterator final : public llvm::mapped_iterator_base<ValueTypeIterator<ValueIteratorT>,ValueIteratorT,Type> {
public: using llvm::mapped_iterator_base<ValueTypeIterator<ValueIteratorT>,ValueIteratorT,Type>::mapped_iterator_base;
 Type mapElement(Value value) const;
};
template<typename ValueRangeT>
class ValueTypeRange final : public llvm::iterator_range<ValueTypeIterator<typename ValueRangeT::iterator>> {
public: using llvm::iterator_range<ValueTypeIterator<typename ValueRangeT::iterator>>::iterator_range;
};
}

#endif
