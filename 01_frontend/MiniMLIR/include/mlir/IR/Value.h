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
  use_range getUses() const;
  using user_iterator=ValueUserIterator<use_iterator,OpOperand>;
  using user_range=llvm::iterator_range<user_iterator>;
  user_range getUsers() const;
  void replaceAllUsesWith(Value newValue) const;
  void replaceAllUsesExcept(Value newValue,Operation *exceptedUser);
  Operation *getDefiningOp() const;
  Block *getParentBlock();
  Location getLoc() const;
public:
  constexpr Value(detail::ValueImpl *impl = nullptr) : impl(impl) {}
  template <typename OpTy> inline OpTy getDefiningOp() const;
  explicit operator bool() const;
  bool operator==(const Value &other) const;
  Type getType() const;
  bool hasOneUse() const;

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
  inline static bool classof(Value value);
  template <typename ToTy,
            typename = typename std::enable_if<std::is_assignable<
                typename ToTy::ValueType &, Ty>::value>::type>
  inline operator ToTy() const;
  inline Ty getType() const;
  inline void setType(Ty ty);
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
  ValueUseIterator &operator++();
  bool operator!=(const ValueUseIterator &rhs) const;
};
class BlockArgument : public Value { public: using Value::Value; Block *getOwner() const; };
class OpResult : public Value { public: using Value::Value; Operation *getOwner() const; };
}

namespace mlir {
template<typename UseIteratorT,typename OperandType>
class ValueUserIterator final : public llvm::mapped_iterator_base<ValueUserIterator<UseIteratorT,OperandType>,UseIteratorT,Operation *> {
public:
 using llvm::mapped_iterator_base<ValueUserIterator<UseIteratorT,OperandType>,UseIteratorT,Operation *>::mapped_iterator_base;
 Operation *mapElement(OperandType &value) const;
 Operation *operator->();
};
}

#endif
