// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations from the local LLVM/MLIR checkout.
#ifndef BUDDY_MINI_MLIR_INTERFACES_CALLINTERFACES_H
#define BUDDY_MINI_MLIR_INTERFACES_CALLINTERFACES_H
#include "mlir/IR/OpDefinition.h"

namespace mlir { namespace detail {
struct CallOpInterfaceInterfaceTraits { struct Concept; template<typename T> class Model; template<typename T> class FallbackModel; template<typename T, typename U> class ExternalModel; };
template<typename T> struct CallOpInterfaceTrait;
}
class CallOpInterface : public OpInterface<CallOpInterface, detail::CallOpInterfaceInterfaceTraits> {
public:
  using OpInterface<CallOpInterface, detail::CallOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::CallOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct CallOpInterfaceTrait : OpInterface<CallOpInterface, CallOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

#endif
