#ifndef FC_LLVM_ADT_SMALLVECTOR_H
#define FC_LLVM_ADT_SMALLVECTOR_H

#include <initializer_list>

#include "llvm/Support/Compiler.h"

namespace llvm {

template <typename T> class ArrayRef;

/* 官方 SmallVector 的默认内联容量由这个类算；模型只需名字与"随 T 计算"的形态。 */
template <typename T> struct CalculateSmallVectorDefaultInlinedElements {
  static const unsigned value = 0;
};

/* 官方 SmallVector<T, N> 继承 SmallVectorImpl<T>；模型摊平成一个类
 * （只保留 pass 用到的成员），模板参数与构造/成员签名按官方。 */
template <typename T,
          unsigned N = CalculateSmallVectorDefaultInlinedElements<T>::value>
class SmallVector {
public:
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef size_t size_type;

  SmallVector();
  SmallVector(const SmallVector &RHS);
  SmallVector(std::initializer_list<T> IL);
  explicit SmallVector(size_type Size);
  SmallVector(size_type Size, const T &Value);
  template <typename ItTy> SmallVector(ItTy S, ItTy E);
  template <typename U> explicit SmallVector(ArrayRef<U> A);

  SmallVector &operator=(const SmallVector &RHS);

  /* 模型只声明不实现（与官方头一致：这些成员在官方也是声明/内联）。
   * 不写内联体是为了让 `SmallVector<T>` 在 T 仍是不完整类型时也能实例化
   * （Frama-Clang 会实例化所有成员，指针减法要求 T 完整）。 */
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  bool empty() const;
  size_type size() const;

  T &back();
  const T &back() const;

  T &operator[](size_type Index);
  const T &operator[](size_type Index) const;

  T *data();
  const T *data() const;

  void reserve(size_type NumElements);
  void push_back(const T &Elt);
  void push_back(T &&Elt);
  void clear();

private:
  T *BeginX;
  T *EndX;
  T *CapacityX;
};

} // namespace llvm
#endif
