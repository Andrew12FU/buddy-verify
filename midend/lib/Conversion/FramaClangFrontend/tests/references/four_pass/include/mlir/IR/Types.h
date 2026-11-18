#ifndef FC_MLIR_IR_TYPES_H
#define FC_MLIR_IR_TYPES_H

#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/TypeID.h"

namespace mlir {

/* 官方 Type 是 impl 指针句柄；模型同样只放一个不透明指针（无实现）。 */
class Type {
public:
  Type() : impl(0) {}
  explicit Type(const void *impl) : impl(impl) {}

  bool operator==(Type other) const { return impl == other.impl; }
  bool operator!=(Type other) const { return impl != other.impl; }
  explicit operator bool() const { return impl != 0; }
  bool operator!() const { return impl == 0; }

  TypeID getTypeID();
  MLIRContext *getContext() const;

private:
  const void *impl;
};

} // namespace mlir
#endif
