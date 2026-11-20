#ifndef FC_MLIR_SUPPORT_LLVM_H
#define FC_MLIR_SUPPORT_LLVM_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"

/* 官方 mlir/Support/LLVM.h 只做 `using llvm::X;` 导入 + 前向声明。
 * Frama-Clang 0.0.20 解析不了类型的 using 声明（Unsupported Type (Using)），
 * 这里用 typedef / 别名模板表达同一件事；
 * std::move / std::nullopt 的替身也放这里（前端没有 <utility>/<optional>）。 */
namespace std {

/* 官方由标准库提供 `T&& move(T&&)`；模型的 sink 都吃左值引用，
 * 且 Frama-Clang 对右值引用支持有限，故这里返回左值引用（等于"不移动"）。 */
template <class T> T &move(T &value) { return value; }

class nullopt_t {
public:
  int reserved;

  nullopt_t(int tag);
};
extern const nullopt_t nullopt;
} // namespace std

namespace mlir {

typedef llvm::StringRef StringRef;
typedef llvm::Twine Twine;

template <class T,
          unsigned N = llvm::CalculateSmallVectorDefaultInlinedElements<T>::value>
using SmallVector = llvm::SmallVector<T, N>;

template <class T> using ArrayRef = llvm::ArrayRef<T>;

template <class T> using MutableArrayRef = llvm::MutableArrayRef<T>;

/* 官方这里是 `using llvm::isa;` 一族（函数 using 声明）。同样为避开前端的
 * using 限制，改成同名转发模板，行为等价。 */
template <typename To, typename From> bool isa(const From &Val) {
  return llvm::isa<To>(Val);
}
template <typename To, typename From> To cast(const From &Val) {
  return llvm::cast<To>(Val);
}
template <typename To, typename From> To dyn_cast(const From &Val) {
  return llvm::dyn_cast<To>(Val);
}
template <typename To, typename From> To cast_or_null(const From &Val) {
  return llvm::cast_or_null<To>(Val);
}
template <typename To, typename From> To dyn_cast_or_null(const From &Val) {
  return llvm::dyn_cast_or_null<To>(Val);
}
template <typename To, typename From> bool isa_and_nonnull(const From &Val) {
  return llvm::isa_and_nonnull<To>(Val);
}

} // namespace mlir
#endif
