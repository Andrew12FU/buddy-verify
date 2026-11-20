#ifndef FC_LLVM_ADT_TWINE_H
#define FC_LLVM_ADT_TWINE_H

#include "llvm/ADT/StringRef.h"

namespace llvm {

/* 官方 Twine 是一棵字符串拼接树；模型只需要"能由字符串隐式构造"这一能力。 */
class Twine {
public:
  Twine();
  Twine(const char *Str);
  Twine(StringRef Str);

private:
  const void *impl;
};

} // namespace llvm
#endif
