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
class RegionBranchOpInterface : public OpInterface<RegionBranchOpInterface, detail::RegionBranchOpInterfaceInterfaceTraits> {
public:
  using OpInterface<RegionBranchOpInterface, detail::RegionBranchOpInterfaceInterfaceTraits>::OpInterface;
  template<typename T> struct Trait : detail::RegionBranchOpInterfaceTrait<T> {};
};
namespace detail { template<typename T> struct RegionBranchOpInterfaceTrait : OpInterface<RegionBranchOpInterface, RegionBranchOpInterfaceInterfaceTraits>::template Trait<T> {}; }
}

#endif
