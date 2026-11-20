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
struct TosaOpInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1347; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
template <typename ConcreteOp> struct TosaOpTrait;
} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1349; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
class TosaOp
    : public ::mlir::OpInterface<TosaOp, detail::TosaOpInterfaceTraits> {
public:
  using ::mlir::OpInterface<TosaOp, detail::TosaOpInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::TosaOpTrait<ConcreteOp> {};
};
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1354; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
template <typename ConcreteOp>
struct TosaOpTrait
    : public ::mlir::OpInterface<
          TosaOp, detail::TosaOpInterfaceTraits>::template Trait<ConcreteOp> {};
} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1357; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
struct QueryProfileInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1359; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
class QueryProfileInterface
    : public ::mlir::OpInterface<QueryProfileInterface,
                                 detail::QueryProfileInterfaceTraits> {
public:
  using ::mlir::OpInterface<QueryProfileInterface,
                            detail::QueryProfileInterfaceTraits>::OpInterface;
};
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1365; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
namespace detail {
struct QueryExtensionInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1367; see docs/PROVENANCE.md.
namespace mlir {
namespace tosa {
class QueryExtensionInterface
    : public ::mlir::OpInterface<QueryExtensionInterface,
                                 detail::QueryExtensionInterfaceTraits> {
public:
  using ::mlir::OpInterface<QueryExtensionInterface,
                            detail::QueryExtensionInterfaceTraits>::OpInterface;
};
} // namespace tosa
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1372; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
namespace tosa {
template <typename ConcreteType>
class TosaResolvableShapeOperands
    : public TraitBase<ConcreteType, TosaResolvableShapeOperands> {};
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
template class Op<
    tosa::ReshapeOp, OpTrait::ZeroRegions, OpTrait::OneResult,
    OpTrait::OneTypedResult<TensorType>::Impl, OpTrait::ZeroSuccessors,
    OpTrait::NOperands<2>::Impl, OpTrait::OpInvariants,
    ConditionallySpeculatable::Trait, OpTrait::AlwaysSpeculatableImplTrait,
    MemoryEffectOpInterface::Trait, InferTypeOpInterface::Trait,
    InferShapedTypeOpInterface::Trait, OpTrait::InferShapedTypeOpAdaptor,
    OpTrait::InferTensorType, tosa::TosaOp::Trait,
    ::mlir::tosa::QueryProfileInterface::Trait,
    ::mlir::tosa::QueryExtensionInterface::Trait,
    OpTrait::tosa::TosaResolvableShapeOperands>;
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
class ReshapeOp
    : public Op<ReshapeOp, OpTrait::ZeroRegions, OpTrait::OneResult,
                OpTrait::OneTypedResult<TensorType>::Impl,
                OpTrait::ZeroSuccessors, OpTrait::NOperands<2>::Impl,
                OpTrait::OpInvariants, ConditionallySpeculatable::Trait,
                OpTrait::AlwaysSpeculatableImplTrait,
                MemoryEffectOpInterface::Trait, InferTypeOpInterface::Trait,
                InferShapedTypeOpInterface::Trait,
                OpTrait::InferShapedTypeOpAdaptor, OpTrait::InferTensorType,
                TosaOp::Trait, ::mlir::tosa::QueryProfileInterface::Trait,
                ::mlir::tosa::QueryExtensionInterface::Trait,
                OpTrait::tosa::TosaResolvableShapeOperands> {
public:
  using Op::Op;
  // Canonical type of mlir::TypedValue<TensorType> in Value.h:494.
  inline ::mlir::detail::TypedValue<::mlir::TensorType> getInput1();
  inline ::mlir::OpOperand &getInput1Mutable();
};
} // namespace tosa
} // namespace mlir

#include "mlir/IR/BuiltinAttributes.h"
namespace mlir {
namespace tosa {
 






 





 





 





} // namespace tosa
} // namespace mlir
#endif
