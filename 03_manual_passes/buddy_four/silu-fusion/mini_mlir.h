#ifndef SILU_MINI_MLIR_H
#define SILU_MINI_MLIR_H
// Conversion-only static API model; no IR semantics, layouts, or ABI guarantee.
// Sources and abstraction limits are recorded in interfaces/README.md.
typedef unsigned long size_t;
typedef long int64_t;
namespace std {
// Canonical libstdc++ string identity used only by default label parameters.
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
// Compiler-recognized initializer_list layout; minimal necessary implementation.
// No standard-library headers or API behavior contracts are imported.
template <typename T> class initializer_list {
public:
  typedef const T *const_iterator;
  typedef unsigned long size_type;
private:
  const_iterator data;
  size_type length;
  constexpr initializer_list(const_iterator p, size_type n) : data(p), length(n) {}
public:
  constexpr initializer_list() noexcept : data(0), length(0) {}
  constexpr size_type size() const noexcept { return length; }
  constexpr const_iterator begin() const noexcept { return data; }
  constexpr const_iterator end() const noexcept { return data + length; }
};
}

namespace std { using nullptr_t = decltype(nullptr); }
namespace llvm {
class StringRef { public: constexpr StringRef(const char *Str); };
struct LogicalResult {};
LogicalResult success(bool IsSuccess = true);
LogicalResult failure(bool IsFailure = true);
bool failed(LogicalResult);
template<class T, unsigned N> class SmallVector;
template<class T> class ArrayRef {
public:
  ArrayRef();
  ArrayRef(const T &OneElt);
  // Real ArrayRef.h pointer/length overload; external implementation, as above.
  constexpr ArrayRef(const T *data, size_t length);
  constexpr ArrayRef(std::initializer_list<T> Vec);
  // Current data()/size() constructor: retain required member-availability checks.
  // Pointer-convertibility/integral predicates are normalized for the three verified SmallVector instances.
  template<class C, class = decltype(static_cast<const C*>(nullptr)->data()),
           class = decltype(static_cast<const C*>(nullptr)->size())>
  constexpr ArrayRef(const C &V);
  const T &operator[](size_t Index) const;
};
// N values here follow the local default sizeof-based calculation, not one N for every T.
template<class T, unsigned N> class SmallVector {
public:
  SmallVector();
  const T *data() const;
  size_t size() const;
  SmallVector(size_t SizeArg, const T &Value);
  SmallVector(std::initializer_list<T> IL);
  void push_back(T Elt); // current trivial AffineExpr specialization
};
template<class To, class From> To cast(const From &Val);
template<class Fn> class function_ref;
template<class Ret, class... Params> class function_ref<Ret(Params...)> {
public:
  function_ref() = default;
  function_ref(std::nullptr_t);
  // External adapter placeholder: omit SFINAE/storage/thunk. Does not invoke callable.
  // A visible definition is necessary for a constructor instantiated with a local closure.
  template<class Callable> function_ref(Callable &&callable) {}
};
}
namespace mlir {
using StringRef = llvm::StringRef;
using LogicalResult = llvm::LogicalResult;
using llvm::success; using llvm::failure; using llvm::failed;
using llvm::ArrayRef; using llvm::cast; using llvm::function_ref;
class MLIRContext; class Operation;
class Location {};
class Type {};
class TensorType : public Type {};
class RankedTensorType : public TensorType {
public:
  int64_t getRank() const;
  ArrayRef<int64_t> getShape() const;
  Type getElementType() const;
};
namespace detail { class ValueImpl; }
class Value {
public:
  constexpr Value(detail::ValueImpl *impl = nullptr);
  explicit operator bool() const;
  bool operator==(const Value &other) const;
  Type getType() const;
  template<class OpTy> OpTy getDefiningOp() const;
  bool hasOneUse() const;
};
namespace detail { template<class Ty> struct TypedValue : Value { Ty getType() const; }; }
class OpResult : public Value {};
class ValueRange {
public:
  ValueRange(const Value &value);
  ValueRange(const std::initializer_list<Value> &values);
  ValueRange(ArrayRef<Value> values = {});
  Value operator[](size_t index) const;
};
class TypeRange {
public:
  TypeRange(ArrayRef<Type> types = {});
  // Current RankedTensorType -> TypeRange forwarding-constructor instance.
  // Omitted availability SFINAE; body only adapts to ArrayRef<Type> in real library.
  template<class Arg> TypeRange(Arg &&arg);
};
class AttributeStorage;
class Attribute { public: constexpr Attribute() = default; Attribute(const AttributeStorage *impl); };
class TypedAttr : public Attribute {
public:
  explicit TypedAttr(Attribute attr);
  // Current FloatAttr trait-enabled inherited constructor; availability constraint omitted.
  template<class T> TypedAttr(T t);
};
class FloatAttr : public Attribute {};
class NamedAttribute {};
class AffineExpr {};
class AffineMap {
public:
  static AffineMap get(unsigned dimCount, unsigned symbolCount, ArrayRef<AffineExpr> results, MLIRContext *context);
};
namespace utils { enum class IteratorType : unsigned { parallel, reduction }; }
// Default Ns verified from real AST before completing this experiment.
template<class T> struct MiniSmallVectorN;
template<> struct MiniSmallVectorN<AffineExpr> { static constexpr unsigned value=6; };
template<> struct MiniSmallVectorN<AffineMap> { static constexpr unsigned value=6; };
template<> struct MiniSmallVectorN<utils::IteratorType> { static constexpr unsigned value=12; };
template<class T, unsigned N=MiniSmallVectorN<T>::value> using SmallVector = llvm::SmallVector<T,N>;
class Builder {
public:
  MLIRContext *getContext() const;
  FloatAttr getFloatAttr(Type type, double value);
  AffineExpr getAffineDimExpr(unsigned position);
};
class OpBuilder : public Builder {};
class RewriterBase : public OpBuilder {
public:
  virtual void replaceOp(Operation *op, ValueRange newValues);
  virtual void eraseOp(Operation *op);
};
class PatternRewriter : public RewriterBase {};
namespace tosa {
class MulOp {
public:
  Location getLoc();
  detail::TypedValue<TensorType> getInput1();
  detail::TypedValue<TensorType> getInput2();
  detail::TypedValue<TensorType> getResult();
  operator Operation*() const;
};
class SigmoidOp {
public:
  SigmoidOp(std::nullptr_t);
  explicit operator bool();
  detail::TypedValue<TensorType> getInput();
  detail::TypedValue<TensorType> getResult();
  operator Operation*() const;
};
}
class PatternBenefit { public: PatternBenefit(unsigned benefit); };
class Pattern {
protected:
  Pattern(StringRef rootName, PatternBenefit benefit, MLIRContext *context, ArrayRef<StringRef> generatedNames = {});
};
class RewritePattern : public Pattern {
public:
  virtual ~RewritePattern();
  virtual LogicalResult matchAndRewrite(Operation*, PatternRewriter&) const=0;
protected: using Pattern::Pattern;
};
namespace detail {
template<class SourceOp> struct OpOrInterfaceRewritePatternBase : RewritePattern {
  using RewritePattern::RewritePattern;
  LogicalResult matchAndRewrite(Operation*, PatternRewriter&) const final;
  virtual LogicalResult matchAndRewrite(SourceOp, PatternRewriter&) const=0;
};
}
template<class SourceOp> struct OpRewritePattern : detail::OpOrInterfaceRewritePatternBase<SourceOp> {
  OpRewritePattern(MLIRContext *context, PatternBenefit benefit=1, ArrayRef<StringRef> generatedNames={});
};
class RewritePatternSet {
public:
  RewritePatternSet(MLIRContext *context);
  template<class... Ts, class ConstructorArg, class... ConstructorArgs>
  RewritePatternSet &add(ConstructorArg &&arg, ConstructorArgs &&...args);
};
class GreedyRewriteConfig {};
class FrozenRewritePatternSet {
public:
  FrozenRewritePatternSet(RewritePatternSet&&, ArrayRef<std::string> disabledPatternLabels={}, ArrayRef<std::string> enabledPatternLabels={});
  ~FrozenRewritePatternSet();
};
LogicalResult applyPatternsGreedily(Operation*, const FrozenRewritePatternSet&, GreedyRewriteConfig config=GreedyRewriteConfig(), bool *changed=nullptr);
class TypeID {};
class Pass {
public:
  virtual ~Pass();
  virtual StringRef getArgument() const;
  virtual StringRef getDescription() const;
  virtual void runOnOperation()=0;
  MLIRContext &getContext();
  void signalPassFailure();
protected: Pass(const Pass&);
};
namespace func { class FuncOp { public: operator Operation*() const; }; }
template<class OpT=void> class OperationPass : public Pass {
protected:
  OpT getOperation();
  OperationPass(TypeID);
  OperationPass(const OperationPass&);
};
template<class PassT, class BaseT> class PassWrapper : public BaseT {
protected: PassWrapper(); PassWrapper(const PassWrapper&);
};
template<class ConcretePass> struct PassRegistration { PassRegistration(); };
namespace tensor {
class EmptyOp {
public:
  static EmptyOp create(OpBuilder &builder, Location location, ArrayRef<int64_t> staticShape, Type elementType, Attribute encoding = {});
  operator detail::TypedValue<TensorType>();
};
}
namespace linalg {
class GenericOp {
public:
  static GenericOp create(OpBuilder &builder, Location location, TypeRange resultTensorTypes, ValueRange inputs, ValueRange outputs, ArrayRef<AffineMap> indexingMaps, ArrayRef<utils::IteratorType> iteratorTypes, function_ref<void(OpBuilder &, Location, ValueRange)> bodyBuild=nullptr, ArrayRef<NamedAttribute> attributes={});
  OpResult getResult(unsigned index);
};
class YieldOp { public: static YieldOp create(OpBuilder&, Location, ValueRange); };
}
namespace arith {
enum class FastMathFlags : unsigned { none=0 };
class ConstantOp {
public: static ConstantOp create(OpBuilder&, Location, TypedAttr);
  operator detail::TypedValue<Type>();
};
class NegFOp {
public: static NegFOp create(OpBuilder&, Location, Value, FastMathFlags fastmath=FastMathFlags::none);
  operator detail::TypedValue<Type>();
};
class AddFOp { public: static AddFOp create(OpBuilder&, Location, Value, Value, FastMathFlags fastmath=FastMathFlags::none); operator detail::TypedValue<Type>(); };
class DivFOp { public: static DivFOp create(OpBuilder&, Location, Value, Value, FastMathFlags fastmath=FastMathFlags::none); operator detail::TypedValue<Type>(); };
class MulFOp { public: static MulFOp create(OpBuilder&, Location, Value, Value, FastMathFlags fastmath=FastMathFlags::none); operator detail::TypedValue<Type>(); };
}
namespace math { class ExpOp {
public: static ExpOp create(OpBuilder&, Location, Value, arith::FastMathFlags fastmath=arith::FastMathFlags::none);
  operator detail::TypedValue<Type>();
}; }
// EXTRA_INTERFACES
}
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CLASS_NAME) static ::mlir::TypeID resolveTypeID();
#endif
