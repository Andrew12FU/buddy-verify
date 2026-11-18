#ifndef LLVM_ADT_STLEXTRAS_H
#define LLVM_ADT_STLEXTRAS_H

#include "llvm/ADT/SmallVector.h"
#include "mlir/IR/Value.h"

namespace llvm {

struct MappedRange {};

template <typename Range, typename F>
MappedRange map_range(const Range &, F) {
  return MappedRange();
}

template <unsigned N, typename Range>
mlir::SmallVector<mlir::Value, N> to_vector(const Range &) {
  return mlir::SmallVector<mlir::Value, N>();
}

} // namespace llvm

#endif
