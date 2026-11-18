// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_DIALECT_LINALG_IR_LINALG_H
#define BUDDY_MINI_MLIR_DIALECT_LINALG_IR_LINALG_H
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/TypeRange.h"

namespace mlir {
namespace utils {
enum class IteratorType : unsigned { parallel, reduction };
}
namespace linalg {
class GenericOp : public Op<GenericOp, OpTrait::VariadicResults> {
public:
  using Op::Op;
  static GenericOp create(
      OpBuilder &builder, Location location, TypeRange resultTensorTypes,
      ValueRange inputs, ValueRange outputs, ArrayRef<AffineMap> indexingMaps,
      ArrayRef<utils::IteratorType> iteratorTypes,
      function_ref<void(OpBuilder &, Location, ValueRange)> bodyBuild = nullptr,
      ArrayRef<NamedAttribute> attributes = {});
};
class YieldOp : public Op<YieldOp> {
public:
  using Op::Op;
  static YieldOp create(OpBuilder &, Location, ValueRange);
};
} // namespace linalg
} // namespace mlir
namespace mlir { namespace linalg { class LinalgDialect : public Dialect { public: explicit LinalgDialect(MLIRContext *context); }; } }
namespace mlir { namespace linalg { namespace detail {
struct LinalgOpInterfaceTraits { struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel; };template<typename T> struct LinalgOpTrait;
}
class LinalgOp : public OpInterface<LinalgOp,detail::LinalgOpInterfaceTraits> { public: using OpInterface<LinalgOp,detail::LinalgOpInterfaceTraits>::OpInterface;template<typename T> struct Trait : detail::LinalgOpTrait<T> {}; };
namespace detail { template<typename T> struct LinalgOpTrait : OpInterface<LinalgOp,LinalgOpInterfaceTraits>::template Trait<T> {}; }
} }
namespace mlir { namespace linalg { namespace detail {
struct ConvolutionOpInterfaceInterfaceTraits { struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel; };template<typename T> struct ConvolutionOpInterfaceTrait;
}
class ConvolutionOpInterface : public OpInterface<ConvolutionOpInterface,detail::ConvolutionOpInterfaceInterfaceTraits> { public: using OpInterface<ConvolutionOpInterface,detail::ConvolutionOpInterfaceInterfaceTraits>::OpInterface;template<typename T> struct Trait : detail::ConvolutionOpInterfaceTrait<T> {}; };
namespace detail { template<typename T> struct ConvolutionOpInterfaceTrait : OpInterface<ConvolutionOpInterface,ConvolutionOpInterfaceInterfaceTraits>::template Trait<T> {}; }
} }
namespace mlir { namespace linalg { namespace detail {
struct FillOpInterfaceInterfaceTraits { struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel; };template<typename T> struct FillOpInterfaceTrait;
}
class FillOpInterface : public OpInterface<FillOpInterface,detail::FillOpInterfaceInterfaceTraits> { public: using OpInterface<FillOpInterface,detail::FillOpInterfaceInterfaceTraits>::OpInterface;template<typename T> struct Trait : detail::FillOpInterfaceTrait<T> {}; };
namespace detail { template<typename T> struct FillOpInterfaceTrait : OpInterface<FillOpInterface,FillOpInterfaceInterfaceTraits>::template Trait<T> {}; }
} }
namespace mlir { namespace linalg {
class DepthwiseConv2DNhwcHwcOp : public ::mlir::Op<DepthwiseConv2DNhwcHwcOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<YieldOp>::Impl, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::DestinationStyleOpInterface::Trait, ::mlir::IndexingMapOpInterface::Trait, ::mlir::linalg::LinalgOp::Trait, ::mlir::ReifyRankedShapedTypeOpInterface::Trait, ::mlir::linalg::ConvolutionOpInterface::Trait> {
public:
 using Op::Op;
 static StringRef getOperationName(); DenseIntElementsAttr getStrides(); DenseIntElementsAttr getDilations();
};
} }
namespace mlir { namespace linalg {
class FillOp : public ::mlir::Op<FillOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<YieldOp>::Impl, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::DestinationStyleOpInterface::Trait, ::mlir::IndexingMapOpInterface::Trait, ::mlir::linalg::LinalgOp::Trait, ::mlir::ReifyRankedShapedTypeOpInterface::Trait, ::mlir::linalg::FillOpInterface::Trait> {
public:
 using Op::Op;
 static StringRef getOperationName();
};
} }


namespace mlir { namespace linalg {
class TransposeOp : public ::mlir::Op<TransposeOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::SingleBlockImplicitTerminator<YieldOp>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::HasRecursiveMemoryEffects, ::mlir::DestinationStyleOpInterface::Trait, ::mlir::IndexingMapOpInterface::Trait, ::mlir::linalg::LinalgOp::Trait, ::mlir::ReifyRankedShapedTypeOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait> {
public: using Op::Op;
 static StringRef getOperationName();
};
} }

#endif
