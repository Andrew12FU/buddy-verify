#ifndef MLIR_IR_TYPEUTILITIES_H
#define MLIR_IR_TYPEUTILITIES_H

namespace mlir {

template <typename To, typename From>
To cast(From) {
  return To();
}

} // namespace mlir

#endif
