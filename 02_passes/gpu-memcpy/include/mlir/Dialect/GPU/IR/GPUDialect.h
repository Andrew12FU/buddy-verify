// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_DIALECT_GPU_IR_GPUDIALECT_H
#define BUDDY_MINI_MLIR_DIALECT_GPU_IR_GPUDIALECT_H
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/TypeRange.h"
namespace mlir { namespace gpu {
class GPUDialect;
enum class AddressSpace : unsigned { Global=1,Workgroup=2,Private=3,Constant=4 };
namespace detail { struct AddressSpaceAttrStorage; struct AsyncOpInterfaceInterfaceTraits {
 struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel; };
 template<typename T> struct AsyncOpInterfaceTrait;
}
class AddressSpaceAttr : public Attribute::AttrBase<AddressSpaceAttr,Attribute,detail::AddressSpaceAttrStorage> {
public:using Base::Base;AddressSpace getValue() const;
};
} }
namespace mlir { namespace gpu {
class AsyncOpInterface : public ::mlir::OpInterface<AsyncOpInterface,detail::AsyncOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<AsyncOpInterface,detail::AsyncOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::AsyncOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct AsyncOpInterfaceTrait : ::mlir::OpInterface<AsyncOpInterface,AsyncOpInterfaceInterfaceTraits>::template Trait<T> {}; }
} }

namespace mlir { namespace gpu {
class AllocOp : public ::mlir::Op<AllocOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::AtLeastNResults<1>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::gpu::AsyncOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static AllocOp create(OpBuilder &builder,Location location,TypeRange resultTypes,ValueRange operands,ArrayRef<NamedAttribute> attributes = {});
};
} }
namespace mlir { namespace gpu {
class DeallocOp : public ::mlir::Op<DeallocOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<1>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::gpu::AsyncOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static DeallocOp create(OpBuilder &builder,Location location,TypeRange asyncToken,ValueRange asyncDependencies,Value memref);
};
} }
namespace mlir { namespace gpu {
class MemcpyOp : public ::mlir::Op<MemcpyOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::gpu::AsyncOpInterface::Trait, ::mlir::MemoryEffectOpInterface::Trait> {
public:
 using Op::Op;
 static MemcpyOp create(OpBuilder &builder,Location location,TypeRange asyncToken,ValueRange asyncDependencies,Value dst,Value src);
};
} }
namespace mlir { namespace gpu {
class LaunchOp : public ::mlir::Op<LaunchOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::VariadicResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::AtLeastNOperands<6>::Impl, ::mlir::OpTrait::AttrSizedOperandSegments, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AffineScope, ::mlir::OpTrait::AutomaticAllocationScope, ::mlir::InferIntRangeInterface::Trait, ::mlir::gpu::AsyncOpInterface::Trait, ::mlir::OpTrait::HasRecursiveMemoryEffects> {
public:
 using Op::Op;
 
};
} }
#endif
