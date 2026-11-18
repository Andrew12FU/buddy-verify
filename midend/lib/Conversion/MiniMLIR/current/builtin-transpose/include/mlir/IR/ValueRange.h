// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MLIR_IR_VALUERANGE_H
#define BUDDY_MINI_MLIR_IR_VALUERANGE_H

#include "mlir/IR/Value.h"
#include "llvm/ADT/PointerUnion.h"

// Provenance: audited Reshape V2 mini, line 1022; see docs/PROVENANCE.md.
namespace mlir {
class OpOperand;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1023; see docs/PROVENANCE.md.
namespace mlir {
class BlockArgument;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1024; see docs/PROVENANCE.md.
namespace mlir {
class OperandRange;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1025; see docs/PROVENANCE.md.
namespace mlir {
class ResultRange;
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1026; see docs/PROVENANCE.md.
namespace mlir {
namespace detail {
class OpResultImpl;
} // namespace detail
} // namespace mlir
// Provenance: audited Reshape V2 mini, line 1027; see docs/PROVENANCE.md.
namespace mlir {
template<typename ValueRangeT> class ValueTypeRange;
class ValueRange final
    : public llvm::detail::indexed_accessor_range_base<
          ValueRange,
          llvm::PointerUnion<const Value *, OpOperand *, detail::OpResultImpl *,
                             const llvm::Repeated<Value> *>,
          Value, Value, Value> {
public:
  using type_range = ValueTypeRange<ValueRange>;
  type_range getTypes() const;
  using OwnerT =
      llvm::PointerUnion<const Value *, OpOperand *, detail::OpResultImpl *,
                         const llvm::Repeated<Value> *>;
  using RangeBaseT::RangeBaseT;
  template <typename Arg,
            typename = std::enable_if_t<
                std::is_constructible<ArrayRef<Value>, Arg>::value &&
                !std::is_convertible<Arg, Value>::value>>
  inline ValueRange(Arg &&arg [[clang::lifetimebound]]);
  inline ValueRange(const Value &value [[clang::lifetimebound]]);
  inline ValueRange(const std::initializer_list<Value> &values
                    [[clang::lifetimebound]]);
  inline ValueRange(ArrayRef<BlockArgument> values);
  ValueRange(ArrayRef<Value> values = {});
  ValueRange(OperandRange values);
  ValueRange(ResultRange values);
  inline ValueRange(const llvm::Repeated<Value> &repeatedValue
                    [[clang::lifetimebound]]);
};
} // namespace mlir

namespace mlir {
class ResultRange final : public llvm::detail::indexed_accessor_range_base<ResultRange, detail::OpResultImpl *, OpResult, OpResult, OpResult> {
public:
  using RangeBaseT::RangeBaseT;
  class UseIterator;
  using use_iterator = UseIterator;
  using use_range = llvm::iterator_range<use_iterator>;
};
}

namespace mlir {
class ResultRange::UseIterator final
    : public llvm::iterator_facade_base<UseIterator, std::forward_iterator_tag, OpOperand> {
public:
  explicit UseIterator(ResultRange results, bool end = false);
   
  OpOperand &operator*() const;
   
private:
  ResultRange::iterator it, endIt;
  Value::use_iterator use;
};
}
#endif
