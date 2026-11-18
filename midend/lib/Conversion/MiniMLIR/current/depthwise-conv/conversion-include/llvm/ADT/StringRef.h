// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_STRINGREF_H
#define BUDDY_MINI_LLVM_ADT_STRINGREF_H

#include "mini/Support/Standard.h"

// Provenance: audited Reshape V2 mini, line 764; see docs/PROVENANCE.md.
namespace llvm {
class [[gsl::Pointer]] StringRef {
public:
  using size_type = size_t;

private:
  const char *Data = nullptr;
  size_t Length = 0;

public:
  StringRef() = default;
  StringRef(std::nullptr_t) = delete;
  constexpr StringRef(const char *Str [[clang::lifetimebound]]);
  constexpr StringRef(const char *data [[clang::lifetimebound]], size_t length);
  StringRef(const std::string &Str);
  constexpr StringRef(std::string_view Str);
};
} // namespace llvm

#endif
