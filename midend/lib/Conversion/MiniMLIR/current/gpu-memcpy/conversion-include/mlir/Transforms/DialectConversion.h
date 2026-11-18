// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_TRANSFORMS_DIALECTCONVERSION_H
#define BUDDY_MINI_MLIR_TRANSFORMS_DIALECTCONVERSION_H
#include "mlir/Rewrite/FrozenRewritePatternSet.h"

namespace llvm {
 
 

} // namespace llvm
namespace mlir {
class TypeConverter {
public:
  using Base = TypeConverter;
  virtual ~TypeConverter() = default;
  TypeConverter() = default;
  TypeConverter(const TypeConverter &other);
  TypeConverter &operator=(const TypeConverter &other);
};
 


 









 








class Diagnostic;
 




 










 




 



} // namespace mlir
#endif
