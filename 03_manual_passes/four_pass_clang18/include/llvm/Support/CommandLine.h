#ifndef FC_LLVM_SUPPORT_COMMANDLINE_H
#define FC_LLVM_SUPPORT_COMMANDLINE_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
namespace cl {

/* 官方形状：desc 是结构体、构造吃 StringRef；init 是**函数模板**，
 * 返回 initializer<Ty>（不是类）。 */
struct desc {
  StringRef Desc;

  desc(StringRef Str) : Desc(Str) {}
};

template <class Ty> struct initializer {
  const Ty &Init;

  initializer(const Ty &Val) : Init(Val) {}
};

template <class Ty> initializer<Ty> init(const Ty &Val) {
  return initializer<Ty>(Val);
}

} // namespace cl
} // namespace llvm
#endif
