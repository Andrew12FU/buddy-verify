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
// Concrete indexed range: ValueRange
class ValueRange;
} // namespace mlir
namespace llvm { namespace detail {
template<>
class indexed_accessor_range_base<::mlir::ValueRange, llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *>, ::mlir::Value, ::mlir::Value, ::mlir::Value> {
public:
  using RangeBaseT = indexed_accessor_range_base;
  ::mlir::Value operator[](long index) const;
   
  class iterator : public indexed_accessor_iterator<iterator,llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *>,::mlir::Value,::mlir::Value,::mlir::Value> {
  public:
    ::mlir::Value operator*() const;
  private:
    iterator(llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *> owner,std::ptrdiff_t curIndex);
    friend indexed_accessor_range_base<::mlir::ValueRange,llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *>,::mlir::Value,::mlir::Value,::mlir::Value>;
  };
  iterator begin() const;
  iterator end() const;
  inline indexed_accessor_range_base(iterator begin, iterator end);
  inline indexed_accessor_range_base(const iterator_range<iterator> &range);
  inline indexed_accessor_range_base(llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *> base, long count);

protected:
  indexed_accessor_range_base(const indexed_accessor_range_base &) = default;
  indexed_accessor_range_base(indexed_accessor_range_base &&) = default;
  indexed_accessor_range_base &
  operator=(const indexed_accessor_range_base &) = default;
  llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *> base;
  long count;
};
} }
namespace mlir {
class ValueRange final
    : public llvm::detail::indexed_accessor_range_base<
          ValueRange,
          llvm::PointerUnion<const Value *, OpOperand *, detail::OpResultImpl *,
                             const llvm::Repeated<Value> *>,
          Value, Value, Value> {
public:
  using type_range = ValueTypeRange<ValueRange>;
   
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
// Concrete indexed range: ResultRange
class ResultRange;
} // namespace mlir
namespace llvm { namespace detail {
template<>
class indexed_accessor_range_base<::mlir::ResultRange, ::mlir::detail::OpResultImpl *, ::mlir::OpResult, ::mlir::OpResult, ::mlir::OpResult> {
public:
  using RangeBaseT = indexed_accessor_range_base;
  ::mlir::OpResult operator[](long index) const;
   
  class iterator : public indexed_accessor_iterator<iterator,::mlir::detail::OpResultImpl *,::mlir::OpResult,::mlir::OpResult,::mlir::OpResult> {
  public:
    ::mlir::OpResult operator*() const;
  private:
    iterator(::mlir::detail::OpResultImpl * owner,std::ptrdiff_t curIndex);
    friend indexed_accessor_range_base<::mlir::ResultRange,::mlir::detail::OpResultImpl *,::mlir::OpResult,::mlir::OpResult,::mlir::OpResult>;
  };
  iterator begin() const;
  iterator end() const;
  inline indexed_accessor_range_base(iterator begin, iterator end);
  inline indexed_accessor_range_base(const iterator_range<iterator> &range);
  inline indexed_accessor_range_base(::mlir::detail::OpResultImpl * base, long count);

protected:
  indexed_accessor_range_base(const indexed_accessor_range_base &) = default;
  indexed_accessor_range_base(indexed_accessor_range_base &&) = default;
  indexed_accessor_range_base &
  operator=(const indexed_accessor_range_base &) = default;
  ::mlir::detail::OpResultImpl * base;
  long count;
};
} }
namespace mlir {
class ResultRange final : public llvm::detail::indexed_accessor_range_base<ResultRange, detail::OpResultImpl *, OpResult, OpResult, OpResult> {
public:
  using RangeBaseT::RangeBaseT;
  class UseIterator final
      : public llvm::iterator_facade_base<UseIterator, std::forward_iterator_tag, OpOperand> {
  public:
    explicit UseIterator(ResultRange results, bool end = false);
     
    OpOperand &operator*() const;
     
  private:
    ResultRange::iterator it, endIt;
    Value::use_iterator use;
  };
  using use_iterator = UseIterator;
  using use_range = llvm::iterator_range<use_iterator>;
};
}

namespace mlir {

}
#endif
