// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_SUPPORT_INTERFACESUPPORT_H
#define BUDDY_MINI_MLIR_SUPPORT_INTERFACESUPPORT_H

#include "mlir/Support/LLVM.h"

// Provenance: audited Reshape V2 mini, line 885; see docs/PROVENANCE.md.
namespace mlir {
namespace OpTrait {
template <typename ConcreteType, template <typename> class TraitType>
class TraitBase {
protected:
   
};
} // namespace OpTrait
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 893; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename ConcreteType, typename ValueT, typename Traits,
          typename BaseType,
          template <typename, template <typename> class> class BaseTrait>
class Interface : public BaseType {
public:
  using Concept = typename Traits::Concept;
  template <typename T> using Model = typename Traits::template Model<T>;
  template <typename T>
  using FallbackModel = typename Traits::template FallbackModel<T>;
  template <typename T, typename U>
  using ExternalModel = typename Traits::template ExternalModel<T, U>;
  using InterfaceBase =
      Interface<ConcreteType, ValueT, Traits, BaseType, BaseTrait>;
  using ValueType = ValueT;
  template <typename ConcreteT>
  struct Trait : public BaseTrait<ConcreteT, Trait> {
    using ModelT = Model<ConcreteT>;
  };
  explicit Interface(ValueT t = ValueT());
  Interface(std::nullptr_t);
  template <typename T,
            std::enable_if_t<std::is_base_of<Trait<T>, T>::value> * = nullptr>
  Interface(T t);
  Interface(ValueT t, const Concept *conceptImpl);
  Interface(ValueT t, std::nullptr_t);

private:
  Concept *conceptImpl;
};
} // namespace detail
} // namespace mlir

#endif
