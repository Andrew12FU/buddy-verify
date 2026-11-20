// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_SMALLVECTOR_H
#define BUDDY_MINI_LLVM_ADT_SMALLVECTOR_H

#include "mini/Support/Standard.h"
#include "llvm/ADT/ArrayRef.h"

// Provenance: audited Reshape V2 mini, line 671; see docs/PROVENANCE.md.
namespace llvm {
template <class Size_T> class SmallVectorBase {
protected:
  void *BeginX;
  Size_T Size = 0, Capacity;
  SmallVectorBase() = delete;
  inline SmallVectorBase(void *FirstEl, size_t TotalCapacity);

public:
  inline size_t size() const;
  inline size_t capacity() const;
  inline bool empty() const;
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 682; see docs/PROVENANCE.md.
namespace llvm {
template <class T>
using SmallVectorSizeType =
    std::conditional_t<sizeof(T) < 4 && sizeof(void *) >= 8, unsigned long,
                       unsigned int>;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 685; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, typename = void>
class SmallVectorTemplateCommon
    : public SmallVectorBase<SmallVectorSizeType<T>> {
  using Base = SmallVectorBase<SmallVectorSizeType<T>>;

protected:
  inline SmallVectorTemplateCommon(size_t SizeArg);

public:
  using size_type = size_t;
  using difference_type = long;
  using value_type = T;
  using iterator = T *;
  using const_iterator = const T *;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using Base::capacity;
  using Base::empty;
  using Base::size;
  inline pointer data();
  inline const_pointer data() const;
  inline iterator begin();
  inline const_iterator begin() const;
   
   
  inline reference operator[](size_type idx);
  inline const_reference operator[](size_type idx) const;
   
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 706; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, bool = std::is_trivially_copy_constructible<T>::value &&
                             std::is_trivially_move_constructible<T>::value &&
                             std::is_trivially_destructible<T>::value>
class SmallVectorTemplateBase;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 710; see docs/PROVENANCE.md.
namespace llvm {
template <typename T>
class SmallVectorTemplateBase<T, true> : public SmallVectorTemplateCommon<T> {
  friend class SmallVectorTemplateCommon<T>;

protected:
  static constexpr bool TakesParamByValue = sizeof(T) <= 2 * sizeof(void *);
  using ValueParamT = std::conditional_t<TakesParamByValue, T, const T &>;
  inline SmallVectorTemplateBase(size_t SizeArg);

public:
   
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 720; see docs/PROVENANCE.md.
namespace llvm {
template <typename T>
class SmallVectorImpl : public SmallVectorTemplateBase<T> {
  using SuperClass = SmallVectorTemplateBase<T>;

public:
  using iterator = typename SuperClass::iterator;
  using const_iterator = typename SuperClass::const_iterator;
  using reference = typename SuperClass::reference;
  using size_type = typename SuperClass::size_type;

protected:
  using SmallVectorTemplateBase<T>::TakesParamByValue;
  using ValueParamT = typename SuperClass::ValueParamT;
  inline explicit SmallVectorImpl(unsigned N);
  inline ~SmallVectorImpl();

public:
  SmallVectorImpl(const SmallVectorImpl &) = delete;
  SmallVectorImpl &operator=(const SmallVectorImpl &RHS);
  SmallVectorImpl &operator=(SmallVectorImpl &&RHS);
   
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 738; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, unsigned N> struct SmallVectorStorage {
  alignas(T) char InlineElts[N * sizeof(T)];
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 741; see docs/PROVENANCE.md.
namespace llvm {
template <typename T> struct alignas(T) SmallVectorStorage<T, 0> {};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 742; see docs/PROVENANCE.md.
namespace llvm {
template <typename T, unsigned N> class SmallVector;
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 743; see docs/PROVENANCE.md.
namespace llvm {
template <typename T> struct CalculateSmallVectorDefaultInlinedElements {
  static constexpr size_t kPreferredSmallVectorSizeof = 64;
  static_assert(sizeof(T) <= 256, "explicit N required for large T");
  static constexpr size_t PreferredInlineBytes =
      kPreferredSmallVectorSizeof - sizeof(SmallVector<T, 0>);
  static constexpr size_t NumElementsThatFit = PreferredInlineBytes / sizeof(T);
  static constexpr size_t value =
      NumElementsThatFit == 0 ? 1 : NumElementsThatFit;
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 750; see docs/PROVENANCE.md.
namespace llvm {
template <typename T,
          unsigned N = CalculateSmallVectorDefaultInlinedElements<T>::value>
class SmallVector : public SmallVectorImpl<T>, SmallVectorStorage<T, N> {
public:
  template<typename ItTy, typename = std::enable_if_t<std::is_convertible<typename std::iterator_traits<ItTy>::iterator_category, std::input_iterator_tag>::value>>
  SmallVector(ItTy S, ItTy E);
  template<typename U,typename=std::enable_if_t<std::is_convertible<U,T>::value>> explicit SmallVector(ArrayRef<U> A);
  inline SmallVector();
  inline ~SmallVector();
  inline explicit SmallVector(size_t SizeArg);
  SmallVector(std::initializer_list<T> IL);
  inline SmallVector(size_t SizeArg, const T &Value);
  inline SmallVector(const SmallVector &RHS);
  inline SmallVector &operator=(const SmallVector &RHS);
  inline SmallVector(SmallVector &&RHS);
  inline SmallVector(SmallVectorImpl<T> &&RHS);
  inline SmallVector &operator=(SmallVector &&RHS);
  inline SmallVector &operator=(SmallVectorImpl<T> &&RHS);
};
} // namespace llvm


namespace llvm {
 

 



}

#endif
