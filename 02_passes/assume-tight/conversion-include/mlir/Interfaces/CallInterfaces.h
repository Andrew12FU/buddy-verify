// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations from the local LLVM/MLIR checkout.
#ifndef BUDDY_MINI_MLIR_INTERFACES_CALLINTERFACES_H
#define BUDDY_MINI_MLIR_INTERFACES_CALLINTERFACES_H
#include "mlir/IR/OpDefinition.h"

namespace mlir { namespace detail {
struct CallOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct CallOpInterfaceTrait;
}
class CallOpInterface;
namespace detail {
template <>
class Interface<::mlir::CallOpInterface, ::mlir::Operation *, ::mlir::detail::CallOpInterfaceInterfaceTraits, ::mlir::OpState, ::mlir::OpTrait::TraitBase> : public ::mlir::OpState {
public:
  using Concept = typename ::mlir::detail::CallOpInterfaceInterfaceTraits::Concept;
  template <typename T> using Model = typename ::mlir::detail::CallOpInterfaceInterfaceTraits::template Model<T>;
  template <typename T>
  using FallbackModel = typename ::mlir::detail::CallOpInterfaceInterfaceTraits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename ::mlir::detail::CallOpInterfaceInterfaceTraits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<::mlir::CallOpInterface, ::mlir::Operation *, ::mlir::detail::CallOpInterfaceInterfaceTraits, ::mlir::OpState, ::mlir::OpTrait::TraitBase>;
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
template class OpInterface<CallOpInterface, detail::CallOpInterfaceInterfaceTraits>;
class CallOpInterface : public OpInterface<CallOpInterface, detail::CallOpInterfaceInterfaceTraits> {
public:
  using OpInterface<CallOpInterface, detail::CallOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::CallOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct CallOpInterfaceTrait : OpInterface<CallOpInterface, CallOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

#endif
