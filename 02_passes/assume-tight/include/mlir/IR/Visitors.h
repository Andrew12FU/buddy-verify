// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_VISITORS_H
#define BUDDY_MINI_MLIR_IR_VISITORS_H

#include "mlir/Support/LLVM.h"
#include "mlir/Support/WalkResult.h"
#include "llvm/ADT/STLFunctionalExtras.h"

// Provenance: audited Reshape V2 mini, line 837; see docs/PROVENANCE.md.
namespace mlir {
enum class WalkOrder { PreOrder, PostOrder };
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 838; see docs/PROVENANCE.md.
namespace mlir {
struct ForwardIterator;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 839; see docs/PROVENANCE.md.
namespace mlir {
class Region;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 840; see docs/PROVENANCE.md.
namespace mlir {
class Block;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 842; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename Ret, typename Arg, typename... Rest>
Arg first_argument_type(Ret (*)(Arg, Rest...));
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 844; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename Ret, typename F, typename Arg, typename... Rest>
Arg first_argument_type(Ret (F::*)(Arg, Rest...));
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 846; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename Ret, typename F, typename Arg, typename... Rest>
Arg first_argument_type(Ret (F::*)(Arg, Rest...) const);
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 848; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename F>
decltype(first_argument_type(&F::operator())) first_argument_type(F);
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 850; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename T>
using first_argument = decltype(first_argument_type(std::declval<T>()));
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 852; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename Iterator>
void walk(Operation *op, llvm::function_ref<void(Operation *)> callback,
          WalkOrder order);
template<typename Iterator> WalkResult walk(Operation *op,llvm::function_ref<WalkResult(Operation *)> callback,WalkOrder order);
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 854; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <
    WalkOrder Order = WalkOrder::PostOrder, typename Iterator = ForwardIterator,
    typename FuncTy, typename ArgT = detail::first_argument<FuncTy>,
    typename RetT = decltype(std::declval<FuncTy>()(std::declval<ArgT>()))>
std::enable_if_t<llvm::is_one_of<ArgT, Operation *, Region *, Block *>::value,
                 RetT>
walk(Operation *op, FuncTy &&callback) {
  return detail::walk<Iterator>(op, llvm::function_ref<RetT(ArgT)>(callback),
                                Order);
}
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 861; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <
    WalkOrder Order = WalkOrder::PostOrder, typename Iterator = ForwardIterator,
    typename FuncTy, typename ArgT = detail::first_argument<FuncTy>,
    typename RetT = decltype(std::declval<FuncTy>()(std::declval<ArgT>()))>
std::enable_if_t<
    !llvm::is_one_of<ArgT, Operation *, Region *, Block *>::value &&
        std::is_same<RetT, void>::value,
    RetT>
walk(Operation *op, FuncTy &&callback) {
  auto wrapperFn = [&](Operation *op) {
    {
      auto derivedOp = dyn_cast<ArgT>(op);
      if (derivedOp)
        callback(derivedOp);
    }
  };
  detail::walk<Iterator>(op, llvm::function_ref<RetT(Operation *)>(wrapperFn),
                         Order);
  return;
}
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 876; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
template <typename FnT>
using walkResultType = decltype(walk(nullptr, std::declval<FnT>()));
} // namespace detail
} // namespace mlir


#endif
