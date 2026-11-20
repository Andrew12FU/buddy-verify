#ifndef LLVM_ADT_STRINGREF_H
#define LLVM_ADT_STRINGREF_H

namespace llvm {

class StringRef {
public:
  StringRef() = default;
  StringRef(const char *) {}
};

} // namespace llvm

#endif
