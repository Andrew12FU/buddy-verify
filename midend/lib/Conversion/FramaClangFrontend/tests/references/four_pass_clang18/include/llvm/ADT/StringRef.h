#ifndef FC_LLVM_ADT_STRINGREF_H
#define FC_LLVM_ADT_STRINGREF_H

#include "llvm/Support/Compiler.h"

namespace llvm {

class StringRef {
public:
  int reserved;

  StringRef();
  StringRef(const char *text);

  const char *data() const;
};

} // namespace llvm
#endif
