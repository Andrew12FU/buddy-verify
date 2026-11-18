#ifndef LLVM_ADT_ARRAYREF_H
#define LLVM_ADT_ARRAYREF_H

#include <cstddef>
#include <initializer_list>

namespace llvm {

template <typename T>
class ArrayRef {
public:
  ArrayRef() = default;

  template <typename... Args>
  ArrayRef(Args &&...) {}

  const T &operator[](std::size_t) const {
    static T value;
    return value;
  }
};

} // namespace llvm

#endif
