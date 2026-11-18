// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_LLVM_ADT_STLFUNCTIONALEXTRAS_H
#define BUDDY_MINI_LLVM_ADT_STLFUNCTIONALEXTRAS_H

#include "llvm/ADT/STLExtras.h"

// Provenance: audited Reshape V2 mini, line 267; see docs/PROVENANCE.md.
namespace llvm {
template <typename Fn> class function_ref;
} // namespace llvm
// Signature/constraints: local llvm/ADT/STLFunctionalExtras.h. The historical
// SiLU mini supplies the external-adapter approach, not its unconstrained ctor.
// See docs/INTERFACE_ABSTRACTION.md for the future ACSL contract boundary.
namespace llvm {
/// Non-owning callable interface; LLVM's storage and dispatch are external.
/// Ret is the callback result type; Params are its argument types, including
/// reference qualifiers. This model preserves overload availability, not layout.
template <typename Ret, typename... Params>
class [[gsl::Pointer]] function_ref<Ret(Params...)> {
public:
  /// Construct an empty view. External declaration: no empty-state semantics
  /// are assumed before a corresponding specification is supplied.
  function_ref();
  function_ref(std::nullptr_t);
  /// Accept a callable with the original constructor's SFINAE constraints.
  /// callable is a non-owning input; the trailing pointers are enable_if gates,
  /// not callback arguments. Constructors have no return value.
  ///
  /// BUDDY_EXTERNAL_TEMPLATE_ADAPTER: a local lambda's type has no external
  /// linkage, so its instantiated constructor needs a visible definition.
  /// The empty body is solely a conversion adapter: it does not store/invoke
  /// callable and must not be used as a no-effect behavioral specification.
  /// Its eventual ACSL model must describe the callable association/lifetime.
  template <typename Callable>
  function_ref(
      Callable &&callable [[clang::lifetimebound]],
      std::enable_if_t<!std::is_same<remove_cvref_t<Callable>,
                                     function_ref>::value> * = nullptr,
      std::enable_if_t<std::is_void<Ret>::value ||
                       std::is_convertible<decltype(std::declval<Callable>()(
                                               std::declval<Params>()...)),
                                           Ret>::value> * = nullptr)
      {}
  /// Invoke with Params and return Ret; effects belong to the external contract.
   
  /// Query whether a callable is associated; behavior is externally specified.
  explicit operator bool() const;
  /// Compare views; equality semantics are left to the external specification.
   
};
} // namespace llvm

#endif
