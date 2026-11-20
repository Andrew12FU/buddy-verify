#ifndef FC_MLIR_IR_TYPERANGE_H
#define FC_MLIR_IR_TYPERANGE_H

#include <initializer_list>

#include "llvm/ADT/ArrayRef.h"

#include "mlir/IR/Types.h"

namespace mlir {

/* 官方 TypeRange 派生自 indexed_accessor_range，可由 ArrayRef<Type> /
 * initializer_list / ValueRange 等构造；模型只保留 pass 用到的两种构造。 */
class TypeRange {
public:
  TypeRange();
  TypeRange(llvm::ArrayRef<Type> types);
  TypeRange(const std::initializer_list<Type> &types);
  /* 官方有"任意可转 ArrayRef 的容器"构造（TypeRange(Arg&&)），
   * 这里用通用容器模板表达，好让 SmallVector<Type> 直接传。 */
  template <typename C> TypeRange(const C &V);

  Type operator[](size_t index) const;
  size_t size() const;

private:
  int reserved;
};

} // namespace mlir
#endif
