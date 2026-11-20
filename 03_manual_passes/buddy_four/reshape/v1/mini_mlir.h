#ifndef RESHAPE_FULL_PASS_MINI_MLIR_H
#define RESHAPE_FULL_PASS_MINI_MLIR_H

// Conversion-only declarations. No input/output or side-effect contracts.
// Only dependencies needed by the current increment are included.
// Empty complete types are opaque stand-ins; real layouts are not reproduced.

// x86_64-conda-linux-gnu: canonical __SIZE_TYPE__ is unsigned long.
typedef unsigned long size_t;

namespace std {
// Native toolchain resolves std::string to this canonical libstdc++ type.
// Only the type identity for FrozenRewritePatternSet default arguments is used.
template <typename T> struct char_traits;
template <typename T> class allocator;
namespace __cxx11 {
template <typename CharT, typename Traits, typename Allocator> class basic_string;
}
using string = __cxx11::basic_string<char, char_traits<char>, allocator<char>>;
template <typename T> struct remove_reference { typedef T type; };
template <typename T> struct remove_reference<T &> { typedef T type; };
template <typename T> struct remove_reference<T &&> { typedef T type; };
template <typename T>
constexpr typename remove_reference<T>::type &&move(T &&value) noexcept;
}

namespace llvm {
// Declaration of the const-From& overload used with MLIR type handles.
// The resolved return type is To (Types.h CastInfo specialization).
// CastInfo/assertion/trait implementation and unused overloads are omitted.
template <typename To, typename From> To dyn_cast(const From &Val);
// Only the pointer-element instantiation is currently required.
// Its real default N resolves to 6 in this target; storage/layout is omitted.
template <typename T> class SmallVectorImpl {
public:
  // ValueParamT is T for the used trivially-copyable pointer element type.
  void push_back(T Elt);
};
template <typename T, unsigned N = 6>
class SmallVector : public SmallVectorImpl<T> {
public:
  SmallVector();
};
class StringRef {
public:
  constexpr StringRef(const char *Str);
};
struct LogicalResult {};
LogicalResult success(bool IsSuccess = true);
LogicalResult failure(bool IsFailure = true);
template <typename T> class ArrayRef {
public:
  ArrayRef();
  // Used generic-container overload from ArrayRef.h:84-94.
  // Availability traits are omitted; the current SmallVector is a valid source.
  template <typename C, typename = void>
  constexpr ArrayRef(const C &V);
};
}

namespace mlir {
using llvm::ArrayRef;
using llvm::SmallVector;
using llvm::dyn_cast;
using LogicalResult = llvm::LogicalResult;
using StringRef = llvm::StringRef;
using llvm::success;
using llvm::failure;

class MLIRContext;
class Operation {
public:
  bool use_empty();
};
class PatternRewriter;
class Type {
public:
  explicit operator bool() const;
  bool operator!() const;
  bool operator!=(Type other) const;
};
// The used Type -> TensorType -> RankedTensorType relation is retained.
// Unused TypeBase storage and shaped traits are not reproduced.
class TensorType : public Type {};
class RankedTensorType : public TensorType {
public:
  size_t getNumDynamicDims() const;
};
class Value {
public:
  template <typename OpTy> OpTy getDefiningOp() const;
};
class ValueRange final {
public:
  ValueRange(const Value &value);
};
namespace detail {
template <typename Ty> struct TypedValue : Value {
  Ty getType() const;
};
}
class OpOperand {
public:
  void assign(Value value);
};
namespace tosa {
class TosaDialect {};
class ReshapeOp {
public:
  // Canonical type of mlir::TypedValue<TensorType> in Value.h:494.
  ::mlir::detail::TypedValue<::mlir::TensorType> getInput1();
  explicit operator bool();
  operator Operation *() const;
  Operation *operator->() const;
  Operation *getOperation();
  ::mlir::OpOperand &getInput1Mutable();
  ::mlir::TensorType getType();
};
}

// Only the used RewriterBase/PatternRewriter hierarchy is retained.
// Unused OpBuilder layers and constructors are omitted.
class RewriterBase {
public:
  virtual void eraseOp(Operation *op);
  virtual void replaceOp(Operation *op, ValueRange newValues);
  virtual void replaceOp(Operation *op, Operation *newOp);
  virtual void startOpModification(Operation *op);
  virtual void finalizeOpModification(Operation *op);

  // A definition is necessary for instantiation with a local lambda type.
  // This is the exact used wrapper body from PatternMatch.h; notification
  // methods remain declarations, with no modeled IR behavior or contracts.
  template <typename CallableT>
  void modifyOpInPlace(Operation *root, CallableT &&callable) {
    startOpModification(root);
    callable();
    finalizeOpModification(root);
  }
};
class PatternRewriter : public RewriterBase {};

class PatternBenefit {
public:
  PatternBenefit(unsigned benefit);
};

class Pattern {
protected:
  Pattern(StringRef rootName, PatternBenefit benefit, MLIRContext *context,
          ArrayRef<StringRef> generatedNames = {});
};

class RewritePattern : public Pattern {
public:
  virtual ~RewritePattern();
  virtual LogicalResult matchAndRewrite(Operation *op,
                                        PatternRewriter &rewriter) const = 0;
protected:
  using Pattern::Pattern;
};

namespace detail {
template <typename SourceOp>
struct OpOrInterfaceRewritePatternBase : public RewritePattern {
  using RewritePattern::RewritePattern;
  LogicalResult matchAndRewrite(Operation *op,
                                PatternRewriter &rewriter) const final;
  virtual LogicalResult matchAndRewrite(SourceOp op,
                                        PatternRewriter &rewriter) const = 0;
};
}

template <typename SourceOp>
struct OpRewritePattern
    : public mlir::detail::OpOrInterfaceRewritePatternBase<SourceOp> {
  using Base = OpRewritePattern;
  OpRewritePattern(MLIRContext *context, PatternBenefit benefit = 1,
                   ArrayRef<StringRef> generatedNames = {});
};

enum class GreedyRewriteStrictness { AnyOp, ExistingAndNewOps, ExistingOps };
enum class GreedySimplifyRegionLevel { Disabled, Normal, Aggressive };
class GreedyRewriteConfig {
public:
  GreedyRewriteConfig &setStrictness(GreedyRewriteStrictness mode);
  GreedyRewriteConfig &enableConstantCSE(bool enable = true);
  GreedyRewriteConfig &enableFolding(bool enable = true);
  GreedyRewriteConfig &setRegionSimplificationLevel(GreedySimplifyRegionLevel level);
};

class RewritePatternSet {
public:
  RewritePatternSet(MLIRContext *context);
  // The used overload's parameter and return types are preserved.
  // SFINAE availability checks, addImpl and construction implementation omitted.
  template <typename... Ts, typename ConstructorArg, typename... ConstructorArgs>
  RewritePatternSet &add(ConstructorArg &&arg, ConstructorArgs &&...args);
};

class FrozenRewritePatternSet {
public:
  FrozenRewritePatternSet(RewritePatternSet &&patterns,
                         ArrayRef<std::string> disabledPatternLabels = {},
                         ArrayRef<std::string> enabledPatternLabels = {});
  ~FrozenRewritePatternSet();
};

LogicalResult applyOpPatternsGreedily(
    ArrayRef<Operation *> ops, const FrozenRewritePatternSet &patterns,
    GreedyRewriteConfig config = GreedyRewriteConfig(),
    bool *changed = nullptr, bool *allErased = nullptr);

// Interfaces extracted from Pass.h. No execution or clone implementation.
template <typename ConcretePass> struct PassRegistration {
  PassRegistration();
};

class DialectRegistry {
public:
  template <typename ConcreteDialect> void insert();
};
class TypeID {};
class Pass {
public:
  virtual ~Pass();
  virtual StringRef getArgument() const;
  virtual StringRef getDescription() const;
  virtual void runOnOperation() = 0;
  virtual void getDependentDialects(DialectRegistry &registry) const;
protected:
  Pass(const Pass &other);
};
enum class WalkOrder { PreOrder, PostOrder };
struct ForwardIterator;
namespace func {
class FuncOp {
public:
  MLIRContext *getContext();
  // Extracted from OpState::walk. Only the current one-argument void callback
  // is modeled: its resolved RetT is void. Trait/SFINAE implementation omitted.
  template <WalkOrder Order = WalkOrder::PostOrder,
            typename Iterator = ForwardIterator, typename FnT,
            typename RetT = void>
  // Local lambda type requires a visible definition for instantiation.
  // Empty conversion stub: no traversal, callback count or IR behavior modeled.
  RetT walk(FnT &&callback) {}
};
}
template <typename OpT = void>
class OperationPass : public Pass {
protected:
  OpT getOperation();
  OperationPass(TypeID passID);
  OperationPass(const OperationPass &);
};
template <typename PassT, typename BaseT>
class PassWrapper : public BaseT {
protected:
  PassWrapper();
  PassWrapper(const PassWrapper &);
};
}

// TypeID.h:331-340: declaration-only extraction of the macro's public API.
// The static local SelfOwningTypeID body and framework static_assert are omitted.
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CLASS_NAME) \
  static ::mlir::TypeID resolveTypeID();

#endif
