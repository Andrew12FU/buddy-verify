// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_VALUE_H
#define BUDDY_MINI_MLIR_IR_VALUE_H

#include "mlir/IR/Types.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/ADT/iterator.h"
#include "mlir/IR/Location.h"

// Provenance: audited Reshape V2 mini, line 1014; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
class ValueImpl;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1015; see docs/PROVENANCE.md.
namespace mlir {
template<typename T> class ValueUseIterator;
template<typename UseIteratorT,typename OperandType> class ValueUserIterator;
class OpOperand;
class Value {
public:
  using use_iterator = ValueUseIterator<OpOperand>;
  using use_range = llvm::iterator_range<use_iterator>;
   
  using user_iterator=ValueUserIterator<use_iterator,OpOperand>;
  using user_range=llvm::iterator_range<user_iterator>;
   
   
   
   
   
   
public:
  constexpr Value(detail::ValueImpl *impl = nullptr) : impl(impl) {}
   
  explicit operator bool() const;
   
  Type getType() const;
   

protected:
  detail::ValueImpl *impl;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1046; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename Ty> struct TypedValue : Value {
  using Value::Value;
  using ValueType = Ty;
   
  template <typename ToTy,
            typename = typename std::enable_if<std::is_assignable<
                typename ToTy::ValueType &, Ty>::value>::type>
  inline operator ToTy() const;
   
   
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1057; see docs/PROVENANCE.md.
namespace mlir {
template <typename Ty, typename Value = mlir::Value>
using TypedValue = std::conditional_t<std::is_same<Ty, mlir::Type>::value,
                                      mlir::Value, detail::TypedValue<Ty>>;
} // namespace mlir

namespace mlir {
namespace detail { class IROperandBase; }
template<typename OperandType> class ValueUseIterator
    : public llvm::iterator_facade_base<ValueUseIterator<OperandType>, std::forward_iterator_tag, OperandType> {
protected:
  detail::IROperandBase *current;
public:
  ValueUseIterator(detail::IROperandBase *use = nullptr);
  OperandType &operator*() const;
   
   
};
class BlockArgument : public Value { public: using Value::Value;   };
class OpResult : public Value { public: using Value::Value;   };
}

namespace mlir {
template<typename UseIteratorT,typename OperandType>
class ValueUserIterator final : public llvm::mapped_iterator_base<ValueUserIterator<UseIteratorT,OperandType>,UseIteratorT,Operation *> {
public:
 using llvm::mapped_iterator_base<ValueUserIterator<UseIteratorT,OperandType>,UseIteratorT,Operation *>::mapped_iterator_base;
  
  
};
}

#endif
