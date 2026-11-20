#ifndef FC_MLIR_IR_VALUERANGE_H
#define FC_MLIR_IR_VALUERANGE_H

#include <initializer_list>

#include "llvm/ADT/ArrayRef.h"

#include "mlir/IR/Value.h"

namespace mlir {

class Operation;

/* 官方 ValueRange：由 Value / initializer_list / ArrayRef / OperandRange /
 * ResultRange / 迭代器区间构造，提供 operator[] 与 size。
 * 另外 3 个多参构造是 Frama-Clang 必需（本前端不接受花括号 initializer_list
 * 出现在变量声明里），官方没有 —— 见 include/FORCED_DEVIATIONS.md。 */
class ValueRange {
public:
  ValueRange();
  ValueRange(const Value &value);
  ValueRange(const std::initializer_list<Value> &values);
  ValueRange(llvm::ArrayRef<Value> values);

  ValueRange(const Value &v0, const Value &v1);
  ValueRange(const Value &v0, const Value &v1, const Value &v2);
  ValueRange(const Value &v0, const Value &v1, const Value &v2,
             const Value &v3);

  Value operator[](size_t index) const;
  size_t size() const;

private:
  int reserved;
};

/* 官方 ResultRange（ValueRange.h里，派生自 indexed_accessor_range）；
 * 模型用指针区间表达 begin/end。 */
class ResultRange {
public:
  ResultRange();
  ResultRange(OpResult result);

  OpResult operator[](size_t index) const;
  size_t size() const;

  OpResult *begin();
  OpResult *end();

private:
  int reserved;
};

/* 官方 OperandRange：operator[] 返回 Value。 */
class OperandRange {
public:
  OperandRange();

  Value operator[](size_t index) const;
  size_t size() const;

  OpOperand *begin();
  OpOperand *end();

private:
  int reserved;
};

/* 官方 MutableOperandRange（可写视图）。 */
class MutableOperandRange {
public:
  MutableOperandRange(Operation *owner);
  MutableOperandRange(OpOperand &opOperand);

  OpOperand &operator[](unsigned index) const;
  size_t size() const;

  OpOperand *begin();
  OpOperand *end();

  void assign(ValueRange values);
  void erase(unsigned subStart, unsigned subLen = 1);

private:
  int reserved;
};

} // namespace mlir
#endif
