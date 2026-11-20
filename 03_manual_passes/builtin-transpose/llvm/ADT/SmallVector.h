#ifndef LLVM_ADT_SMALLVECTOR_H
#define LLVM_ADT_SMALLVECTOR_H

#include <cstddef>

namespace llvm {

template <typename T, unsigned N>
class SmallVector {
public:
  SmallVector() = default;

  T &operator[](std::size_t) {
    static T value;
    return value;
  }
};

} // namespace llvm

#endif
