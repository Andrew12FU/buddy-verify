#ifndef FC_LLVM_SUPPORT_LOGICALRESULT_H
#define FC_LLVM_SUPPORT_LOGICALRESULT_H

/* 官方 21.x：LogicalResult 实体在 llvm/Support/LogicalResult.h，
 * mlir/Support/LogicalResult.h 只是转发头。 */
namespace llvm {

class LogicalResult {
public:
  static LogicalResult success(bool isSuccess = true);
  static LogicalResult failure(bool isFailure = true);

  bool succeeded() const { return isSuccess; }
  bool failed() const { return !isSuccess; }

private:
  LogicalResult(bool isSuccess) : isSuccess(isSuccess) {}

  bool isSuccess;
};

inline LogicalResult success(bool isSuccess = true) {
  return LogicalResult::success(isSuccess);
}

inline LogicalResult failure(bool isFailure = true) {
  return LogicalResult::failure(isFailure);
}

inline bool succeeded(LogicalResult result) { return result.succeeded(); }

inline bool failed(LogicalResult result) { return result.failed(); }

} // namespace llvm
#endif