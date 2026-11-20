#ifndef FC_MLIR_IR_VALUE_H
#define FC_MLIR_IR_VALUE_H

#include "llvm/ADT/STLExtras.h"

#include "mlir/IR/Location.h"
#include "mlir/IR/Types.h"

namespace mlir {

class Operation;
class Block;
class OpOperand;

/* 官方 use_iterator / user_iterator 是迭代器类。
 * 模型只用它们做 begin/end/自增/解引用，内部用裸指针实现（行为一致）。 */
class use_iterator {
public:
  use_iterator() : cur(0) {}
  use_iterator(OpOperand *cur) : cur(cur) {}

  OpOperand &operator*() const;
  OpOperand *operator->() const;
  use_iterator &operator++();

  bool operator==(const use_iterator &rhs) const { return cur == rhs.cur; }
  bool operator!=(const use_iterator &rhs) const { return cur != rhs.cur; }

private:
  OpOperand *cur;
};

using use_range = llvm::iterator_range<use_iterator>;

class user_iterator {
public:
  user_iterator();

  Operation *operator*() const;
  user_iterator &operator++();

  bool operator==(const user_iterator &rhs) const;
  bool operator!=(const user_iterator &rhs) const;

private:
  const OpOperand *cur;
};

using user_range = llvm::iterator_range<user_iterator>;

/* 官方 Value：impl 指针句柄。 */
class Value {
public:
  Value() : impl(0) {}

  explicit operator bool() const { return impl != 0; }
  bool operator!() const { return impl == 0; }

  Type getType() const;
  Location getLoc() const;

  Operation *getDefiningOp() const;
  Block *getParentBlock();

  use_range getUses() const;
  user_range getUsers() const;

  void replaceAllUsesWith(Value newValue);
  void replaceAllUsesExcept(Value newValue, Operation *exceptedUser);

  template <typename OpTy> OpTy getDefiningOp() const { return OpTy(); }

private:
  void *impl;
};

/* 官方 OpOperand 定义在 Value.h：一个操作数 = 值 + 所属操作。 */
class OpOperand {
public:
  OpOperand();

  Value get() const;
  void set(Value newValue);
  Operation *getOwner() const;
  bool is(Value value) const;

private:
  Value value;
  Operation *owner;
};

class BlockArgument : public Value {
public:
  BlockArgument();
  BlockArgument(const Value &other);

  Block *getOwner() const;

  static bool classof(Value value);
};

class OpResult : public Value {
public:
  OpResult();
  OpResult(const Value &other);

  Operation *getOwner() const;

  static bool classof(Value value);
};

} // namespace mlir
#endif
