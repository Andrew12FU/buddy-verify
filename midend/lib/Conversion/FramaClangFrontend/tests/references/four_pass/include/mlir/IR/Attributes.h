#ifndef FC_MLIR_IR_ATTRIBUTES_H
#define FC_MLIR_IR_ATTRIBUTES_H

#include "mlir/IR/Types.h"

namespace mlir {

/* 官方 Attribute 同样是 impl 指针句柄。 */
class Attribute {
public:
  Attribute() : impl(0) {}
  explicit Attribute(const void *impl) : impl(impl) {}

  bool operator==(Attribute other) const { return impl == other.impl; }
  bool operator!=(Attribute other) const { return impl != other.impl; }
  explicit operator bool() const { return impl != 0; }
  bool operator!() const { return impl == 0; }

  TypeID getTypeID();
  MLIRContext *getContext() const;

private:
  const void *impl;
};

} // namespace mlir
#endif
