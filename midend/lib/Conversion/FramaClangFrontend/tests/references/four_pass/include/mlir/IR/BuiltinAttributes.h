#ifndef FC_MLIR_IR_BUILTINATTRIBUTES_H
#define FC_MLIR_IR_BUILTINATTRIBUTES_H

#include "llvm/ADT/STLExtras.h"

#include "mlir/IR/Attributes.h"

namespace mlir {

/* 官方 TypedAttr：带类型的属性基类。 */
class TypedAttr : public Attribute {
public:
  TypedAttr();
  TypedAttr(const Attribute &other) : Attribute(other) {}

  Type getType() const;
};

/* 官方 StridedLayoutAttr。 */
class StridedLayoutAttr : public Attribute {
public:
  StridedLayoutAttr();
  StridedLayoutAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);

  llvm::ArrayRef<int64_t> getStrides() const;
  int64_t getOffset() const;

  static StridedLayoutAttr get(MLIRContext *context, int64_t offset,
                               llvm::ArrayRef<int64_t> strides);
};

/* 官方 DenseArrayAttr 家族（DenseI64ArrayAttr 用 asArrayRef<T>()）。 */
class DenseI64ArrayAttr : public Attribute {
public:
  DenseI64ArrayAttr();
  DenseI64ArrayAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);

  template <typename T> llvm::ArrayRef<T> asArrayRef() const;

  int64_t size() const;
};

/* 官方 DenseElementsAttr 的遍历器（getValues<T>() 返回它的 iterator_range）。 */
template <typename T> class ElementIterator {
public:
  T operator*() const;
  ElementIterator &operator++();

  bool operator==(const ElementIterator &rhs) const;
  bool operator!=(const ElementIterator &rhs) const;

private:
  const void *impl;
};

class DenseElementsAttr : public Attribute {
public:
  DenseElementsAttr();
  DenseElementsAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);

  template <typename T> llvm::iterator_range<ElementIterator<T>> getValues() const;
};

/* 官方 DenseIntElementsAttr : DenseTypedElementsAttr（模型按用到的最小链：
 * DenseIntElementsAttr : DenseElementsAttr : TypedAttr : Attribute）。 */
class DenseIntElementsAttr : public DenseElementsAttr {
public:
  DenseIntElementsAttr();
  DenseIntElementsAttr(const Attribute &other) : DenseElementsAttr(other) {}

  static bool classof(Attribute attribute);
};

class IntegerAttr : public Attribute {
public:
  IntegerAttr();
  IntegerAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);

  int64_t getInt() const;
};

/* 官方 MemRefLayoutAttrInterface 是生成的 AttributeInterface：
 * 可由任意 Attribute 构造（模型保留这条隐式转换）。 */
class MemRefLayoutAttrInterface : public Attribute {
public:
  MemRefLayoutAttrInterface();
  MemRefLayoutAttrInterface(const Attribute &other) : Attribute(other) {}
};

} // namespace mlir
#endif
