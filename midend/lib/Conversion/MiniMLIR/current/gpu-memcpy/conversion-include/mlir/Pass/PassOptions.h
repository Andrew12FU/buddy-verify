// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_PASS_PASSOPTIONS_H
#define BUDDY_MINI_MLIR_PASS_PASSOPTIONS_H
#include "llvm/Support/CommandLine.h"
namespace mlir { namespace detail {
class PassOptions {
public:
 template<typename DataType,typename OptionParser=llvm::cl::parser<DataType>>
 class Option : public llvm::cl::opt<DataType,false,OptionParser> {
 public:
  using llvm::cl::opt<DataType,false,OptionParser>::operator=;
  template<typename... Args> Option(PassOptions &parent,StringRef arg,Args &&...args);
 };
};
} }

#endif
