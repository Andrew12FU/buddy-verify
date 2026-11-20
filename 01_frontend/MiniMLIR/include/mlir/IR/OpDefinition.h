// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_OPDEFINITION_H
#define BUDDY_MINI_MLIR_IR_OPDEFINITION_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Operation.h"

// Provenance: audited Reshape V2 mini, line 1239; see docs/PROVENANCE.md.
namespace mlir {
class OpState {
public:
  inline explicit operator bool();
  inline operator Operation *() const;
  inline Operation *operator->() const;
  inline Operation *getOperation();
  inline MLIRContext *getContext();
  Location getLoc();
  inline explicit OpState(Operation *state);
  template <WalkOrder Order = WalkOrder::PostOrder,
            typename Iterator = ForwardIterator, typename FnT,
            typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 1,
                   RetT>
  walk(FnT &&callback) {
    return state->walk<Order, Iterator>(std::forward<FnT>(callback));
  }
  template <typename FnT, typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 2,
                   RetT>
  walk(FnT &&callback);

private:
  Operation *state;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1263; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ResultType> class OneTypedResult {
public:
  template <typename ConcreteType>
  class Impl : public TraitBase<ConcreteType,
                                OneTypedResult<ResultType>::template Impl> {
  public:
    inline ::mlir::TypedValue<ResultType> getResult();
    inline operator ::mlir::TypedValue<ResultType>();
    inline ResultType getType();
  };
};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1274; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
namespace detail {
template <typename ConcreteType, template <typename> class TraitType>
struct MultiOperandTraitBase : public TraitBase<ConcreteType, TraitType> {
 unsigned getNumOperands(); Value getOperand(unsigned i);
};
} // namespace detail
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1277; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <unsigned N> class NOperands {
public:
  static_assert(N > 1, "use ZeroOperands/OneOperand for N < 2");
  template <typename ConcreteType>
  class Impl
      : public detail::MultiOperandTraitBase<ConcreteType,
                                             NOperands<N>::template Impl> {};
};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1282; see docs/PROVENANCE.md.
namespace mlir {
template <typename ConcreteType, template <typename T> class... Traits>
class Op : public OpState, public Traits<ConcreteType>... {
public:
  using OpState::getOperation;
  using ConcreteOpType = ConcreteType;
  inline explicit Op();
  inline Op(std::nullptr_t);
  inline explicit Op(Operation *state);
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1292; see docs/PROVENANCE.md.
namespace mlir {
template <typename ConcreteType, typename Traits>
class OpInterface
    : public detail::Interface<ConcreteType, Operation *, Traits,
                               Op<ConcreteType>, OpTrait::TraitBase> {
public:
  using Base = OpInterface<ConcreteType, Traits>;
  using InterfaceBase = detail::Interface<ConcreteType, Operation *, Traits,
                                          Op<ConcreteType>, OpTrait::TraitBase>;
  using InterfaceBase::InterfaceBase;
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1301; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class ZeroRegions : public TraitBase<ConcreteType, ZeroRegions> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1302; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class OneResult : public TraitBase<ConcreteType, OneResult> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1304; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class ZeroSuccessors : public TraitBase<ConcreteType, ZeroSuccessors> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1305; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class OpInvariants : public TraitBase<ConcreteType, OpInvariants> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1306; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct ConditionallySpeculatableInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1307; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct ConditionallySpeculatableTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1309; see docs/PROVENANCE.md.
namespace mlir {
class ConditionallySpeculatable
    : public ::mlir::OpInterface<
          ConditionallySpeculatable,
          detail::ConditionallySpeculatableInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      ConditionallySpeculatable,
      detail::ConditionallySpeculatableInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::ConditionallySpeculatableTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1314; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct ConditionallySpeculatableTrait
    : public ::mlir::OpInterface<
          ConditionallySpeculatable,
          detail::ConditionallySpeculatableInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1315; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
struct AlwaysSpeculatableImplTrait
    : public TraitBase<ConcreteType, AlwaysSpeculatableImplTrait> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1316; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct MemoryEffectOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1317; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct MemoryEffectOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1319; see docs/PROVENANCE.md.
namespace mlir {
class MemoryEffectOpInterface
    : public ::mlir::OpInterface<
          MemoryEffectOpInterface,
          detail::MemoryEffectOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      MemoryEffectOpInterface,
      detail::MemoryEffectOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::MemoryEffectOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1324; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct MemoryEffectOpInterfaceTrait
    : public ::mlir::OpInterface<
          MemoryEffectOpInterface,
          detail::MemoryEffectOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1325; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct InferTypeOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1326; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct InferTypeOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1328; see docs/PROVENANCE.md.
namespace mlir {
class InferTypeOpInterface
    : public ::mlir::OpInterface<InferTypeOpInterface,
                                 detail::InferTypeOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      InferTypeOpInterface,
      detail::InferTypeOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::InferTypeOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1333; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct InferTypeOpInterfaceTrait
    : public ::mlir::OpInterface<InferTypeOpInterface,
                                 detail::InferTypeOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1334; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct InferShapedTypeOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1335; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct InferShapedTypeOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1337; see docs/PROVENANCE.md.
namespace mlir {
class InferShapedTypeOpInterface
    : public ::mlir::OpInterface<
          InferShapedTypeOpInterface,
          detail::InferShapedTypeOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      InferShapedTypeOpInterface,
      detail::InferShapedTypeOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::InferShapedTypeOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1342; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct InferShapedTypeOpInterfaceTrait
    : public ::mlir::OpInterface<
          InferShapedTypeOpInterface,
          detail::InferShapedTypeOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1343; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class InferShapedTypeOpAdaptor
    : public TraitBase<ConcreteType, InferShapedTypeOpAdaptor> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1344; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class InferTensorType : public TraitBase<ConcreteType, InferTensorType> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1373; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class OneRegion : public TraitBase<ConcreteType, OneRegion> { public: Region &getRegion(); };
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1374; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class ZeroResults : public TraitBase<ConcreteType, ZeroResults> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1375; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class ZeroOperands : public TraitBase<ConcreteType, ZeroOperands> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1376; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct BytecodeOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1377; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct BytecodeOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1379; see docs/PROVENANCE.md.
namespace mlir {
class BytecodeOpInterface
    : public ::mlir::OpInterface<BytecodeOpInterface,
                                 detail::BytecodeOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      BytecodeOpInterface,
      detail::BytecodeOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::BytecodeOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1384; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct BytecodeOpInterfaceTrait
    : public ::mlir::OpInterface<BytecodeOpInterface,
                                 detail::BytecodeOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1385; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class AffineScope : public TraitBase<ConcreteType, AffineScope> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1386; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class AutomaticAllocationScope
    : public TraitBase<ConcreteType, AutomaticAllocationScope> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1387; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct SymbolOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1388; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct SymbolOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1390; see docs/PROVENANCE.md.
namespace mlir {
class SymbolOpInterface
    : public ::mlir::OpInterface<SymbolOpInterface,
                                 detail::SymbolOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      SymbolOpInterface, detail::SymbolOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::SymbolOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1395; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct SymbolOpInterfaceTrait
    : public ::mlir::OpInterface<SymbolOpInterface,
                                 detail::SymbolOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1396; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct ArgAndResultAttrsOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1397; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct ArgAndResultAttrsOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1399; see docs/PROVENANCE.md.
namespace mlir {
class ArgAndResultAttrsOpInterface
    : public ::mlir::OpInterface<
          ArgAndResultAttrsOpInterface,
          detail::ArgAndResultAttrsOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      ArgAndResultAttrsOpInterface,
      detail::ArgAndResultAttrsOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::ArgAndResultAttrsOpInterfaceTrait<ConcreteOp> {
  };
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1404; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct ArgAndResultAttrsOpInterfaceTrait
    : public ::mlir::OpInterface<
          ArgAndResultAttrsOpInterface,
          detail::ArgAndResultAttrsOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1405; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct CallableOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1406; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct CallableOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1408; see docs/PROVENANCE.md.
namespace mlir {
class CallableOpInterface
    : public ::mlir::OpInterface<CallableOpInterface,
                                 detail::CallableOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      CallableOpInterface,
      detail::CallableOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::CallableOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1413; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct CallableOpInterfaceTrait
    : public ::mlir::OpInterface<CallableOpInterface,
                                 detail::CallableOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1414; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct FunctionOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1415; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct FunctionOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1417; see docs/PROVENANCE.md.
namespace mlir {
class FunctionOpInterface
    : public ::mlir::OpInterface<FunctionOpInterface,
                                 detail::FunctionOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      FunctionOpInterface,
      detail::FunctionOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::FunctionOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1422; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct FunctionOpInterfaceTrait
    : public ::mlir::OpInterface<FunctionOpInterface,
                                 detail::FunctionOpInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {
 bool isExternal(); void setType(Type newType);
 unsigned getNumArguments(); BlockArgument getArgument(unsigned idx);
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1423; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class IsIsolatedFromAbove
    : public TraitBase<ConcreteType, IsIsolatedFromAbove> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1424; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct OpAsmOpInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1425; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct OpAsmOpInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1427; see docs/PROVENANCE.md.
namespace mlir {
class OpAsmOpInterface
    : public ::mlir::OpInterface<OpAsmOpInterface,
                                 detail::OpAsmOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      OpAsmOpInterface, detail::OpAsmOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::OpAsmOpInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1432; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct OpAsmOpInterfaceTrait
    : public ::mlir::OpInterface<
          OpAsmOpInterface,
          detail::OpAsmOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1612; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
struct NoRegionArguments : public TraitBase<ConcreteType, NoRegionArguments> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1613; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class NoTerminator : public TraitBase<ConcreteType, NoTerminator> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1614; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
struct SingleBlock : public TraitBase<ConcreteType, SingleBlock> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1615; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class SymbolTable : public TraitBase<ConcreteType, SymbolTable> {};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1616; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType>
class HasOnlyGraphRegion : public TraitBase<ConcreteType, HasOnlyGraphRegion> {
};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1617; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
struct RegionKindInterfaceInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T, typename U> class ExternalModel;
};
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1618; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp> struct RegionKindInterfaceTrait;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1620; see docs/PROVENANCE.md.
namespace mlir {
class RegionKindInterface
    : public ::mlir::OpInterface<RegionKindInterface,
                                 detail::RegionKindInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<
      RegionKindInterface,
      detail::RegionKindInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::RegionKindInterfaceTrait<ConcreteOp> {};
};
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1625; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteOp>
struct RegionKindInterfaceTrait
    : public ::mlir::OpInterface<RegionKindInterface,
                                 detail::RegionKindInterfaceInterfaceTraits>::
          template Trait<ConcreteOp> {};
} // namespace detail
} // namespace mlir

namespace mlir {
namespace OpTrait { template<typename T> class OneOperand : public TraitBase<T, OneOperand> {}; }
class OpFoldResult : public llvm::PointerUnion<Attribute, Value> {
public:
  using PointerUnion::PointerUnion;
};
}

namespace mlir { namespace OpTrait {
template<typename T> class VariadicOperands : public detail::MultiOperandTraitBase<T, VariadicOperands> {};
template<typename T> class AttrSizedOperandSegments : public TraitBase<T, AttrSizedOperandSegments> {};
template<typename T> struct MemRefsNormalizable : public TraitBase<T, MemRefsNormalizable> {};
template<typename T> class InferTypeOpAdaptor : public TraitBase<T, InferTypeOpAdaptor> {};
template<typename T> class ReturnLike : public TraitBase<T, ReturnLike> {};
template<typename T> class IsTerminator : public TraitBase<T, IsTerminator> {};
namespace detail { template<typename T,template<typename> class TraitType> struct MultiResultTraitBase : public TraitBase<T,TraitType> { Value getResult(unsigned i); }; }
template<typename T> class VariadicResults : public detail::MultiResultTraitBase<T,VariadicResults> {};
template<unsigned N> class AtLeastNResults { public: template<typename T> class Impl : public detail::MultiResultTraitBase<T, Impl> {}; };
template<unsigned N> class AtLeastNOperands { public: template<typename T> class Impl : public detail::MultiOperandTraitBase<T, Impl> {}; };
template<typename... ParentOpTypes> class HasParent { public: template<typename T> class Impl : public TraitBase<T, Impl> {}; };
} }

namespace mlir { namespace detail {
struct ViewLikeOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct ViewLikeOpInterfaceTrait;
}
class ViewLikeOpInterface : public OpInterface<ViewLikeOpInterface, detail::ViewLikeOpInterfaceInterfaceTraits> {
public:
  using OpInterface<ViewLikeOpInterface, detail::ViewLikeOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::ViewLikeOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct ViewLikeOpInterfaceTrait : OpInterface<ViewLikeOpInterface, ViewLikeOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace detail {
struct MemorySpaceCastConsumerOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct MemorySpaceCastConsumerOpInterfaceTrait;
}
class MemorySpaceCastConsumerOpInterface : public OpInterface<MemorySpaceCastConsumerOpInterface, detail::MemorySpaceCastConsumerOpInterfaceInterfaceTraits> {
public:
  using OpInterface<MemorySpaceCastConsumerOpInterface, detail::MemorySpaceCastConsumerOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::MemorySpaceCastConsumerOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct MemorySpaceCastConsumerOpInterfaceTrait : OpInterface<MemorySpaceCastConsumerOpInterface, MemorySpaceCastConsumerOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace detail {
struct OffsetSizeAndStrideOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct OffsetSizeAndStrideOpInterfaceTrait;
}
class OffsetSizeAndStrideOpInterface : public OpInterface<OffsetSizeAndStrideOpInterface, detail::OffsetSizeAndStrideOpInterfaceInterfaceTraits> {
public:
  using OpInterface<OffsetSizeAndStrideOpInterface, detail::OffsetSizeAndStrideOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::OffsetSizeAndStrideOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct OffsetSizeAndStrideOpInterfaceTrait : OpInterface<OffsetSizeAndStrideOpInterface, OffsetSizeAndStrideOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace detail {
struct RegionBranchTerminatorOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct RegionBranchTerminatorOpInterfaceTrait;
}
class RegionBranchTerminatorOpInterface : public OpInterface<RegionBranchTerminatorOpInterface, detail::RegionBranchTerminatorOpInterfaceInterfaceTraits> {
public:
  using OpInterface<RegionBranchTerminatorOpInterface, detail::RegionBranchTerminatorOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::RegionBranchTerminatorOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct RegionBranchTerminatorOpInterfaceTrait : OpInterface<RegionBranchTerminatorOpInterface, RegionBranchTerminatorOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace OpTrait {
template<typename T> class SameOperandsElementType : public TraitBase<T, SameOperandsElementType> {};
template<typename T> class SameOperandsShape : public TraitBase<T, SameOperandsShape> {};
template<typename T> class SameOperandsAndResultShape : public TraitBase<T, SameOperandsAndResultShape> {};
} }

namespace mlir { namespace detail {
struct CastOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct CastOpInterfaceTrait;
}
class CastOpInterface : public OpInterface<CastOpInterface, detail::CastOpInterfaceInterfaceTraits> {
public:
  using OpInterface<CastOpInterface, detail::CastOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::CastOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct CastOpInterfaceTrait : OpInterface<CastOpInterface, CastOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace detail {
struct AlignmentAttrOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct AlignmentAttrOpInterfaceTrait;
}
class AlignmentAttrOpInterface : public OpInterface<AlignmentAttrOpInterface, detail::AlignmentAttrOpInterfaceInterfaceTraits> {
public:
  using OpInterface<AlignmentAttrOpInterface, detail::AlignmentAttrOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::AlignmentAttrOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct AlignmentAttrOpInterfaceTrait : OpInterface<AlignmentAttrOpInterface, AlignmentAttrOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

namespace mlir { namespace detail {
struct InferIntRangeInterfaceInterfaceTraits { struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel; };
template<typename T> struct InferIntRangeInterfaceTrait;
}
class InferIntRangeInterface : public OpInterface<InferIntRangeInterface,detail::InferIntRangeInterfaceInterfaceTraits> {
public:
 using OpInterface<InferIntRangeInterface,detail::InferIntRangeInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::InferIntRangeInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct InferIntRangeInterfaceTrait : OpInterface<InferIntRangeInterface,InferIntRangeInterfaceInterfaceTraits>::template Trait<T> {}; }
namespace OpTrait { template<typename T> class HasRecursiveMemoryEffects : public TraitBase<T,HasRecursiveMemoryEffects> {}; }
}

namespace mlir { namespace detail {
struct SymbolUserOpInterfaceInterfaceTraits { struct Concept;template<typename T> class Model;template<typename T> class FallbackModel;template<typename T,typename U> class ExternalModel; };
template<typename T> struct SymbolUserOpInterfaceTrait;
}
class SymbolUserOpInterface : public OpInterface<SymbolUserOpInterface,detail::SymbolUserOpInterfaceInterfaceTraits> {
public: using OpInterface<SymbolUserOpInterface,detail::SymbolUserOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::SymbolUserOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct SymbolUserOpInterfaceTrait : OpInterface<SymbolUserOpInterface,SymbolUserOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace OpTrait {
template<typename T> class Elementwise : public TraitBase<T,Elementwise> {};
template<typename T> class Scalarizable : public TraitBase<T,Scalarizable> {};
template<typename T> class Vectorizable : public TraitBase<T,Vectorizable> {};
template<typename T> class Tensorizable : public TraitBase<T,Tensorizable> {};
template<typename T> class IsCommutative : public TraitBase<T,IsCommutative> {};
template<typename T> class SameOperandsAndResultType : public TraitBase<T,SameOperandsAndResultType> {};
template<typename T> class SameTypeOperands : public TraitBase<T,SameTypeOperands> {};
template<typename T> class RecursivelySpeculatableImplTrait : public TraitBase<T,RecursivelySpeculatableImplTrait> {};
template<typename T> class HasParallelRegion : public TraitBase<T,HasParallelRegion> {};
template<unsigned N> class NRegions { public: template<typename T> class Impl : public TraitBase<T,Impl> {}; };
template<typename Terminator> class SingleBlockImplicitTerminator { public: template<typename T> class Impl : public TraitBase<T,Impl> {}; };
} }
namespace mlir { namespace detail {
struct VectorUnrollOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct VectorUnrollOpInterfaceTrait;
}
class VectorUnrollOpInterface : public OpInterface<VectorUnrollOpInterface,detail::VectorUnrollOpInterfaceInterfaceTraits> {
public: using OpInterface<VectorUnrollOpInterface,detail::VectorUnrollOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::VectorUnrollOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct VectorUnrollOpInterfaceTrait : OpInterface<VectorUnrollOpInterface,VectorUnrollOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct InferIntDivisibilityOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct InferIntDivisibilityOpInterfaceTrait;
}
class InferIntDivisibilityOpInterface : public OpInterface<InferIntDivisibilityOpInterface,detail::InferIntDivisibilityOpInterfaceInterfaceTraits> {
public: using OpInterface<InferIntDivisibilityOpInterface,detail::InferIntDivisibilityOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::InferIntDivisibilityOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct InferIntDivisibilityOpInterfaceTrait : OpInterface<InferIntDivisibilityOpInterface,InferIntDivisibilityOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct LoopLikeOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct LoopLikeOpInterfaceTrait;
}
class LoopLikeOpInterface : public OpInterface<LoopLikeOpInterface,detail::LoopLikeOpInterfaceInterfaceTraits> {
public: using OpInterface<LoopLikeOpInterface,detail::LoopLikeOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::LoopLikeOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct LoopLikeOpInterfaceTrait : OpInterface<LoopLikeOpInterface,LoopLikeOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct PromotableRegionOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct PromotableRegionOpInterfaceTrait;
}
class PromotableRegionOpInterface : public OpInterface<PromotableRegionOpInterface,detail::PromotableRegionOpInterfaceInterfaceTraits> {
public: using OpInterface<PromotableRegionOpInterface,detail::PromotableRegionOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::PromotableRegionOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct PromotableRegionOpInterfaceTrait : OpInterface<PromotableRegionOpInterface,PromotableRegionOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct DestinationStyleOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct DestinationStyleOpInterfaceTrait;
}
class DestinationStyleOpInterface : public OpInterface<DestinationStyleOpInterface,detail::DestinationStyleOpInterfaceInterfaceTraits> {
public: using OpInterface<DestinationStyleOpInterface,detail::DestinationStyleOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::DestinationStyleOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct DestinationStyleOpInterfaceTrait : OpInterface<DestinationStyleOpInterface,DestinationStyleOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct InParallelOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct InParallelOpInterfaceTrait;
}
class InParallelOpInterface : public OpInterface<InParallelOpInterface,detail::InParallelOpInterfaceInterfaceTraits> {
public: using OpInterface<InParallelOpInterface,detail::InParallelOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::InParallelOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct InParallelOpInterfaceTrait : OpInterface<InParallelOpInterface,InParallelOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct ShapedDimOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct ShapedDimOpInterfaceTrait;
}
class ShapedDimOpInterface : public OpInterface<ShapedDimOpInterface,detail::ShapedDimOpInterfaceInterfaceTraits> {
public: using OpInterface<ShapedDimOpInterface,detail::ShapedDimOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::ShapedDimOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct ShapedDimOpInterfaceTrait : OpInterface<ShapedDimOpInterface,ShapedDimOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct IndexingMapOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct IndexingMapOpInterfaceTrait;
}
class IndexingMapOpInterface : public OpInterface<IndexingMapOpInterface,detail::IndexingMapOpInterfaceInterfaceTraits> {
public: using OpInterface<IndexingMapOpInterface,detail::IndexingMapOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::IndexingMapOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct IndexingMapOpInterfaceTrait : OpInterface<IndexingMapOpInterface,IndexingMapOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}
namespace mlir { namespace detail {
struct ReifyRankedShapedTypeOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; };
template<typename T> struct ReifyRankedShapedTypeOpInterfaceTrait;
}
class ReifyRankedShapedTypeOpInterface : public OpInterface<ReifyRankedShapedTypeOpInterface,detail::ReifyRankedShapedTypeOpInterfaceInterfaceTraits> {
public: using OpInterface<ReifyRankedShapedTypeOpInterface,detail::ReifyRankedShapedTypeOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::ReifyRankedShapedTypeOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct ReifyRankedShapedTypeOpInterfaceTrait : OpInterface<ReifyRankedShapedTypeOpInterface,ReifyRankedShapedTypeOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}


namespace mlir { 
namespace detail { struct VectorTransferOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; }; template<typename T> struct VectorTransferOpInterfaceTrait; }
class VectorTransferOpInterface : public OpInterface<VectorTransferOpInterface,detail::VectorTransferOpInterfaceInterfaceTraits> {
public: using OpInterface<VectorTransferOpInterface,detail::VectorTransferOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::VectorTransferOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct VectorTransferOpInterfaceTrait : OpInterface<VectorTransferOpInterface,VectorTransferOpInterfaceInterfaceTraits>::template Trait<T> {}; }
 }


namespace mlir { namespace vector {
namespace detail { struct MaskableOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T,typename U> class ExternalModel; }; template<typename T> struct MaskableOpInterfaceTrait; }
} }
namespace mlir { namespace vector {
class MaskableOpInterface : public OpInterface<MaskableOpInterface,detail::MaskableOpInterfaceInterfaceTraits> {
public: using OpInterface<MaskableOpInterface,detail::MaskableOpInterfaceInterfaceTraits>::OpInterface;
 template<typename T> struct Trait : detail::MaskableOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct MaskableOpInterfaceTrait : OpInterface<MaskableOpInterface,MaskableOpInterfaceInterfaceTraits>::template Trait<T> {}; }
} }

#endif
