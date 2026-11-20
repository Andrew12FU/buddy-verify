#ifndef FC_MLIR_SUPPORT_TYPEID_H
#define FC_MLIR_SUPPORT_TYPEID_H

namespace mlir {

/* 官方 mlir/Support/TypeID.h：不透明类型标识 + 每类一份的 TypeID 宏。 */
class TypeID {
public:
  TypeID();
  explicit TypeID(const void *storage);

  bool operator==(TypeID other) const { return storage == other.storage; }
  bool operator!=(TypeID other) const { return storage != other.storage; }

private:
  const void *storage;
};

} // namespace mlir

#ifndef MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(x)
#endif

#endif
