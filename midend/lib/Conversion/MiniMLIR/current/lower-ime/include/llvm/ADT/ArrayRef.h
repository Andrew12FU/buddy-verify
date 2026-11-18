// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_ARRAYREF_H
#define BUDDY_MINI_LLVM_ADT_ARRAYREF_H

#include "mini/Support/Standard.h"

// Provenance: audited Reshape V2 mini, line 794; see docs/PROVENANCE.md.
namespace llvm {
template <typename T> class [[gsl::Pointer]] [[nodiscard]] ArrayRef {
public:
  using value_type = T;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = const_pointer;
  using const_iterator = const_pointer;
  using size_type = size_t;
  using difference_type = long;

private:
  const T *Data = nullptr;
  size_type Length = 0;

public:
  ArrayRef() = default;
  inline ArrayRef(const T &OneElt [[clang::lifetimebound]]);
  constexpr ArrayRef(const T *data [[clang::lifetimebound]], size_t length);
  constexpr ArrayRef(const T *begin [[clang::lifetimebound]], const T *end);
  template <
      typename C,
      typename = std::enable_if_t<
          std::conjunction<
              std::is_convertible<decltype(std::declval<const C &>().data()) *,
                                  const T *const *>,
              std::is_integral<decltype(std::declval<const C &>().size())>>::
              value,
          void>>
  constexpr ArrayRef(const C &V);
  template <size_t N>
  constexpr ArrayRef(const T (&Arr [[clang::lifetimebound]])[N]);
  constexpr ArrayRef(std::initializer_list<T> Vec [[clang::lifetimebound]]);
  const T *data() const;
  inline size_t size() const;
  inline bool empty() const;
   
   
   
   
};
} // namespace llvm

namespace llvm {
template<typename T> class MutableArrayRef : public ArrayRef<T> {
public:
  using iterator = T *;
  using ArrayRef<T>::ArrayRef;
   
   
   
};
}

namespace llvm {
 
template<typename T> inline bool operator==(ArrayRef<T> LHS, ArrayRef<T> RHS);
}

#endif
