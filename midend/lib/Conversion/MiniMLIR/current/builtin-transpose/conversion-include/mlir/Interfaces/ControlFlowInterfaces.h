// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations from the local LLVM/MLIR checkout.
#ifndef BUDDY_MINI_MLIR_INTERFACES_CONTROLFLOWINTERFACES_H
#define BUDDY_MINI_MLIR_INTERFACES_CONTROLFLOWINTERFACES_H
#include "mlir/IR/OpDefinition.h"

namespace mlir { namespace detail {
 
 
}
 




namespace detail {   }
}

namespace mlir { namespace detail {
struct RegionBranchOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct RegionBranchOpInterfaceTrait;
}
class RegionBranchOpInterface;
namespace detail {
template <>
class Interface<::mlir::RegionBranchOpInterface, ::mlir::Operation *, ::mlir::detail::RegionBranchOpInterfaceInterfaceTraits, ::mlir::OpState, ::mlir::OpTrait::TraitBase> : public ::mlir::OpState {
public:
  using Concept = typename ::mlir::detail::RegionBranchOpInterfaceInterfaceTraits::Concept;
  template <typename T> using Model = typename ::mlir::detail::RegionBranchOpInterfaceInterfaceTraits::template Model<T>;
  template <typename T>
  using FallbackModel = typename ::mlir::detail::RegionBranchOpInterfaceInterfaceTraits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename ::mlir::detail::RegionBranchOpInterfaceInterfaceTraits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<::mlir::RegionBranchOpInterface, ::mlir::Operation *, ::mlir::detail::RegionBranchOpInterfaceInterfaceTraits, ::mlir::OpState, ::mlir::OpTrait::TraitBase>;
  using ValueType = ::mlir::Operation *;
  template <typename ConcreteT>
  struct Trait : public ::mlir::OpTrait::TraitBase<ConcreteT, Trait> {
    using ModelT = Model<ConcreteT>;
  };
  explicit Interface(::mlir::Operation * t = ValueType());
  Interface(std::nullptr_t);
  template <typename T,
            std::enable_if_t<std::is_base_of<Trait<T>, T>::value> * = nullptr>
  Interface(T t);
  Interface(::mlir::Operation * t, const Concept *conceptImpl);
  Interface(::mlir::Operation * t, std::nullptr_t);

private:
  Concept *conceptImpl;
};
}
template class OpInterface<RegionBranchOpInterface, detail::RegionBranchOpInterfaceInterfaceTraits>;
class RegionBranchOpInterface : public OpInterface<RegionBranchOpInterface, detail::RegionBranchOpInterfaceInterfaceTraits> {
public:
  using OpInterface<RegionBranchOpInterface, detail::RegionBranchOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::RegionBranchOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct RegionBranchOpInterfaceTrait : OpInterface<RegionBranchOpInterface, RegionBranchOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

#endif
