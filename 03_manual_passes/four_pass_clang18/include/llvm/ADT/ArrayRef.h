#ifndef FC_LLVM_ADT_ARRAYREF_H
#define FC_LLVM_ADT_ARRAYREF_H

#include <initializer_list>

#include "llvm/Support/Compiler.h"

namespace llvm {

/* 官方形状：不可变视图，字段是 (Data, Length)。
 * 构造来自 (指针,长度)、(首,尾)、initializer_list，以及任意"有 data()/size() 的容器"。
 * 官方那个容器构造在 21.x 带 SFINAE 约束，模型为便于 Frama-Clang 省掉约束。 */
template <typename T> class ArrayRef {
public:
  typedef const T *iterator;
  typedef const T *const_iterator;
  typedef size_t size_type;

  ArrayRef() : Data(0), Length(0) {}
  ArrayRef(const T *data, size_t length) : Data(data), Length(length) {}
  ArrayRef(const T *begin, const T *end) : Data(begin), Length(end - begin) {}
  ArrayRef(const std::initializer_list<T> &Vec)
      : Data(Vec.begin()), Length(Vec.size()) {}
  template <typename C>
  ArrayRef(const C &V) : Data(V.data()), Length(V.size()) {}

  /* 只声明不实现，理由同 SmallVector：Frama-Clang 会实例化所有成员，
   * 指针运算要求元素类型完整。 */
  iterator begin() const;
  iterator end() const;
  bool empty() const;
  size_type size() const;
  const T *data() const;

  const T &front() const;
  const T &back() const;
  const T &operator[](size_type Index) const;

  bool equals(ArrayRef RHS) const;

private:
  const T *Data;
  size_type Length;
};

/* 官方写法：非成员运算符，两边按值传。 */
template <typename T> bool operator==(ArrayRef<T> LHS, ArrayRef<T> RHS) {
  return LHS.equals(RHS);
}
template <typename T> bool operator!=(ArrayRef<T> LHS, ArrayRef<T> RHS) {
  return !(LHS == RHS);
}

/* 官方 MutableArrayRef 是 ArrayRef 的可变版本（同在本头）。 */
template <typename T> class MutableArrayRef : public ArrayRef<T> {
public:
  typedef T *iterator;
  typedef const T *const_iterator;

  MutableArrayRef() : ArrayRef<T>() {}
  MutableArrayRef(T *data, size_t length) : ArrayRef<T>(data, length) {}

  T *data() const { return const_cast<T *>(ArrayRef<T>::data()); }
  iterator begin() const { return const_cast<T *>(ArrayRef<T>::data()); }
  iterator end() const { return begin() + ArrayRef<T>::size(); }
  T &operator[](size_t Index) const { return begin()[Index]; }
};

} // namespace llvm
#endif
