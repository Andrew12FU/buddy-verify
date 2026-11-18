#ifndef FC_MLIR_IR_BUILTINTYPES_H
#define FC_MLIR_IR_BUILTINTYPES_H

#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Types.h"

namespace mlir {

/* ShapedType / 各具体类型：官方 classof 都按 getTypeID 判，模型只声明（无实现）。 */
class ShapedType : public Type {
public:
  ShapedType();
  ShapedType(const Type &other) : Type(other) {}

  static bool classof(Type type);
  static bool isDynamic(int64_t value) { return value < 0; }

  Type getElementType() const;
};

class IntegerType : public Type {
public:
  IntegerType();
  IntegerType(const Type &other) : Type(other) {}

  static bool classof(Type type);
};

class FloatType : public Type {
public:
  FloatType();
  FloatType(const Type &other) : Type(other) {}

  static bool classof(Type type);
};

class VectorType : public Type {
public:
  VectorType();
  VectorType(const Type &other) : Type(other) {}

  static bool classof(Type type);

  static VectorType get(llvm::ArrayRef<int64_t> shape, Type elementType,
                        llvm::ArrayRef<bool> scalableDims =
                            llvm::ArrayRef<bool>());

  Type getElementType() const;
};

class FunctionType : public Type {
public:
  FunctionType();
  FunctionType(const Type &other) : Type(other) {}

  static bool classof(Type type);

  llvm::ArrayRef<Type> getResults() const;
  llvm::ArrayRef<Type> getInputs() const;
};

class MemRefType : public Type {
public:
  MemRefType();
  MemRefType(const Type &other) : Type(other) {}

  static bool classof(Type type);

  bool hasRank() const;
  int64_t getRank() const;
  MemRefLayoutAttrInterface getLayout() const;
  llvm::ArrayRef<int64_t> getShape() const;
  Type getElementType() const;
  MLIRContext *getContext() const;
  Attribute getMemorySpace() const;

  static MemRefType get(llvm::ArrayRef<int64_t> shape, Type elementType,
                        MemRefLayoutAttrInterface layout =
                            MemRefLayoutAttrInterface(),
                        Attribute memorySpace = Attribute());

  /* 官方的第二个重载：layout 也可以直接用 AffineMap 给。 */
  static MemRefType get(llvm::ArrayRef<int64_t> shape, Type elementType,
                        AffineMap layout, Attribute memorySpace);
};

} // namespace mlir
#endif
