// LLVM/MLIR 真实头文件入口：所有抽取片段由 AST 来源位置取得。
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/LogicalResult.h"
llvm::LogicalResult inspect_result(bool fail) { return llvm::failure(fail); }
bool inspect_status(llvm::LogicalResult result) { return llvm::failed(result); }
int inspect_range(const int *data, size_t size) {
  llvm::ArrayRef<int> range(data, size);
  return range.size() ? range[0] : 0;
}
