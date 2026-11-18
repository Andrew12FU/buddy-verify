// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_DIALECT_ARITH_IR_ARITH_H
#define BUDDY_MINI_MLIR_DIALECT_ARITH_IR_ARITH_H
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir {
namespace arith {
class ArithDialect;
 
class ConstantOp : public Op<ConstantOp, OpTrait::OneTypedResult<Type>::Impl> {
public:
  using Op::Op;
  static ConstantOp create(OpBuilder &, Location, TypedAttr);
};
 





 





 





 





} // namespace arith
} // namespace mlir
namespace mlir { namespace arith { enum class CmpIPredicate : uint64_t { eq=0,ne=1,slt=2,sle=3,sgt=4,sge=5,ult=6,ule=7,ugt=8,uge=9 }; enum class IntegerOverflowFlags : uint32_t { none=0,nsw=1,nuw=2 }; } }
namespace mlir { namespace arith { namespace detail { struct ArithIntegerOverflowFlagsInterfaceInterfaceTraits {struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel;};template<typename T> struct ArithIntegerOverflowFlagsInterfaceTrait; }
class ArithIntegerOverflowFlagsInterface : public OpInterface<ArithIntegerOverflowFlagsInterface,detail::ArithIntegerOverflowFlagsInterfaceInterfaceTraits> {public: using OpInterface<ArithIntegerOverflowFlagsInterface,detail::ArithIntegerOverflowFlagsInterfaceInterfaceTraits>::OpInterface;template<typename T> struct Trait : detail::ArithIntegerOverflowFlagsInterfaceTrait<T> {};};
namespace detail { template<typename T> struct ArithIntegerOverflowFlagsInterfaceTrait : OpInterface<ArithIntegerOverflowFlagsInterface,ArithIntegerOverflowFlagsInterfaceInterfaceTraits>::template Trait<T> {}; }
} }
namespace mlir { namespace arith {
class AddIOp : public ::mlir::Op<AddIOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::Type>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::IsCommutative, ::mlir::InferIntDivisibilityOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::InferIntRangeInterface::Trait, ::mlir::arith::ArithIntegerOverflowFlagsInterface::Trait, ::mlir::OpTrait::SameOperandsAndResultType, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::OpTrait::Elementwise, ::mlir::OpTrait::Scalarizable, ::mlir::OpTrait::Vectorizable, ::mlir::OpTrait::Tensorizable, ::mlir::InferTypeOpInterface::Trait> {
public:
 using Op::Op;
 static AddIOp create(OpBuilder &,Location,Value,Value,IntegerOverflowFlags flags = IntegerOverflowFlags::none);
};
} }
namespace mlir { namespace arith {
class MulIOp : public ::mlir::Op<MulIOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::Type>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::IsCommutative, ::mlir::InferIntDivisibilityOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::InferIntRangeInterface::Trait, ::mlir::arith::ArithIntegerOverflowFlagsInterface::Trait, ::mlir::OpTrait::SameOperandsAndResultType, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::OpTrait::Elementwise, ::mlir::OpTrait::Scalarizable, ::mlir::OpTrait::Vectorizable, ::mlir::OpTrait::Tensorizable, ::mlir::InferTypeOpInterface::Trait> {
public:
 using Op::Op;
 static MulIOp create(OpBuilder &,Location,Value,Value,IntegerOverflowFlags flags = IntegerOverflowFlags::none);
};
} }
namespace mlir { namespace arith {
class CmpIOp : public ::mlir::Op<CmpIOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::Type>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::InferIntRangeInterface::Trait, ::mlir::ConditionallySpeculatable::Trait, ::mlir::OpTrait::AlwaysSpeculatableImplTrait, ::mlir::MemoryEffectOpInterface::Trait, ::mlir::OpTrait::SameTypeOperands, ::mlir::VectorUnrollOpInterface::Trait, ::mlir::OpTrait::Elementwise, ::mlir::OpTrait::Scalarizable, ::mlir::OpTrait::Vectorizable, ::mlir::OpTrait::Tensorizable, ::mlir::InferTypeOpInterface::Trait> {
public:
 using Op::Op;
 static CmpIOp create(OpBuilder &,Location,CmpIPredicate,Value,Value);
};
} }


namespace mlir { namespace arith {  
















 } }

#endif
