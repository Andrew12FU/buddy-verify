// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_DIALECT_TOSA_IR_TOSAOPS_H
#define BUDDY_MINI_MLIR_DIALECT_TOSA_IR_TOSAOPS_H

#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"

// Provenance: audited Reshape V2 mini, line 1346; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
 





} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1347; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
 
} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1349; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
 






} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1354; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
 



} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1357; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
 





} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1359; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
 






} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1365; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
 





} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1367; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
 






} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1372; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
namespace tosa {
 


} // namespace tosa
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1433; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
class ReshapeOp;
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1434; see docs/PROVENANCE.md.
namespace mlir {
 










} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1446; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
 







} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1453; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
 

















} // namespace tosa
} // namespace mlir

#include "mlir/IR/BuiltinAttributes.h"
namespace mlir {
namespace tosa {
 






class MulOp : public Op<MulOp, OpTrait::OneTypedResult<TensorType>::Impl> {
public:
  using Op::Op;
  ::mlir::detail::TypedValue<TensorType> getInput1();
  ::mlir::detail::TypedValue<TensorType> getInput2();
};
 





class SigmoidOp
    : public Op<SigmoidOp, OpTrait::OneTypedResult<TensorType>::Impl> {
public:
  using Op::Op;
  // Conversion view: inherited nullptr constructor with its default restored.
  SigmoidOp() = default;
  inline SigmoidOp(std::nullptr_t);

  ::mlir::detail::TypedValue<TensorType> getInput();
};
} // namespace tosa
} // namespace mlir
#endif
