// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_SUPPORT_LOGICALRESULT_H
#define BUDDY_MINI_LLVM_SUPPORT_LOGICALRESULT_H

// Provenance: audited Reshape V2 mini, line 779; see docs/PROVENANCE.md.
namespace llvm {
struct [[nodiscard]] LogicalResult {
public:
   
   
   
   

private:
  inline LogicalResult(bool IsSuccess);
  bool IsSuccess;
};
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 789; see docs/PROVENANCE.md.
namespace llvm {
inline LogicalResult success(bool IsSuccess = true);
} // namespace llvm
// Provenance: audited Reshape V2 mini, line 790; see docs/PROVENANCE.md.
namespace llvm {
inline LogicalResult failure(bool IsFailure = true);
} // namespace llvm

namespace llvm {
inline bool failed(LogicalResult Result);
}
#endif
