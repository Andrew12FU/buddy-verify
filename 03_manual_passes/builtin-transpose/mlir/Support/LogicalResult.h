#ifndef MLIR_SUPPORT_LOGICALRESULT_H
#define MLIR_SUPPORT_LOGICALRESULT_H

namespace mlir {

class LogicalResult {
public:
  LogicalResult() = default;
  explicit LogicalResult(bool ok) : ok_(ok) {}
  bool ok() const { return ok_; }

private:
  bool ok_ = false;
};

inline LogicalResult failure() { return LogicalResult(false); }
inline LogicalResult success() { return LogicalResult(true); }
inline bool failed(LogicalResult result) { return !result.ok(); }

} // namespace mlir

#endif
