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
class TosaDialect : public ::mlir::Dialect {
  explicit TosaDialect(::mlir::MLIRContext *context);
   
  friend class ::mlir::MLIRContext;

public:
  ~TosaDialect() override;
};
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
class MatMulOp
    : public Op<MatMulOp, OpTrait::OneTypedResult<TensorType>::Impl> {
public:
  using Op::Op;
  ::mlir::detail::TypedValue<TensorType> getA();
  ::mlir::detail::TypedValue<TensorType> getB();
};
class MulOp : public Op<MulOp, OpTrait::OneTypedResult<TensorType>::Impl> {
public:
  using Op::Op;
   
   
};
class ConstOp : public Op<ConstOp, OpTrait::OneTypedResult<TensorType>::Impl> {
public:
  using Op::Op;
  static ConstOp create(OpBuilder &builder, Location location, Type output,
                        ElementsAttr values);
};
 





} // namespace tosa
} // namespace mlir
#endif
