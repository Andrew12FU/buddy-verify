#ifndef MATMUL_SCALAR_FULL_PASS_MINI_MLIR_H
#define MATMUL_SCALAR_FULL_PASS_MINI_MLIR_H
// Conversion-only API declarations; no input/output behavior contracts.
// Real signatures and required type relations; layouts/traits are omitted.
// Sources: local MLIR PatternMatch.h, Value.h, OpDefinition.h and TosaOps.h.inc.
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
namespace llvm {
class StringRef { public: constexpr StringRef(const char *Str); };
struct LogicalResult {};
LogicalResult success(bool IsSuccess = true);
LogicalResult failure(bool IsFailure = true);
template <typename T> class ArrayRef {
public:
  ArrayRef();
  ArrayRef(const T &OneElt);
  const T &operator[](size_t Index) const;
  constexpr ArrayRef(std::initializer_list<T> Vec);
};
template <typename To, typename From> To dyn_cast(const From &Val);
}
namespace mlir {
// Same-type aliases avoid the previously localized UsingType handling error.
using StringRef = llvm::StringRef;
using LogicalResult = llvm::LogicalResult;
using llvm::success;
using llvm::failure;
using llvm::ArrayRef;
using llvm::dyn_cast;
class MLIRContext;
class Operation;
class Location {};
class OpBuilder {};
class AttributeStorage;
class Attribute {
public:
  constexpr Attribute() = default;
  Attribute(const AttributeStorage *impl);
};
class Type {
public:
  explicit operator bool() const;
  bool operator!() const;
};
class ShapedType {
public:
  ShapedType(Type ty);
  static constexpr bool isDynamic(int64_t dValue);
};
class IntegerType : public Type {
public:
  enum SignednessSemantics { Signless, Signed, Unsigned };
  static IntegerType get(MLIRContext *context, unsigned width,
                         SignednessSemantics signedness = Signless);
};
class TensorType : public Type {};
class RankedTensorType : public TensorType {
public:
  int64_t getRank() const;
  ::llvm::ArrayRef<int64_t> getShape() const;
  static RankedTensorType get(ArrayRef<int64_t> shape, Type elementType,
                              Attribute encoding = {});
};
class IntegerAttr : public Attribute {
public:
  static IntegerAttr get(Type type, int64_t value);
};
// ShapedType is a type interface; retain its used implicit Type constructor.
class DenseElementsAttr : public Attribute {
public:
  static DenseElementsAttr get(ShapedType type, ArrayRef<Attribute> values);
};
class Value {};
namespace detail {
template <typename Ty> struct TypedValue : Value {
  Ty getType() const;
};
}
namespace tosa {
class TosaDialect {};
class MatMulOp {
public:
  ::mlir::detail::TypedValue<::mlir::TensorType> getA();
  ::mlir::detail::TypedValue<::mlir::TensorType> getB();
  // OneTypedResult<TensorType>::Impl in OpDefinition.h:715.
  ::mlir::TensorType getType();
  MLIRContext *getContext();
  Location getLoc();
  operator Operation *() const;
};
}
class RewriterBase : public OpBuilder {
public:
  template <typename OpTy, typename... Args>
  OpTy replaceOpWithNewOp(Operation *op, Args &&...args);
};
class PatternRewriter : public RewriterBase {};
class ElementsAttr : public Attribute {
public:
  ElementsAttr(Attribute attr);
};
namespace tosa {
class MulOp {};
class ConstOp {
public:
  ::mlir::detail::TypedValue<::mlir::TensorType> getResult();
  static ConstOp create(OpBuilder &builder, Location location, Type output,
                        ElementsAttr values);
};
}
class PatternBenefit { public: PatternBenefit(unsigned benefit); };
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
class RewritePatternSet {
public:
  RewritePatternSet(MLIRContext *context);
  // Availability SFINAE and construction implementation omitted.
  template <typename... Ts, typename ConstructorArg, typename... ConstructorArgs>
  RewritePatternSet &add(ConstructorArg &&arg, ConstructorArgs &&...args);
};
enum class GreedyRewriteStrictness { AnyOp, ExistingAndNewOps, ExistingOps };
class GreedyRewriteConfig {
public:
  GreedyRewriteConfig &setStrictness(GreedyRewriteStrictness mode);
};
class FrozenRewritePatternSet {
public:
  FrozenRewritePatternSet(RewritePatternSet &&patterns,
                         ArrayRef<std::string> disabledPatternLabels = {},
                         ArrayRef<std::string> enabledPatternLabels = {});
  ~FrozenRewritePatternSet();
};
// Used Operation* overload; external inline region traversal is not modeled.
LogicalResult applyPatternsGreedily(
    Operation *op, const FrozenRewritePatternSet &patterns,
    GreedyRewriteConfig config = GreedyRewriteConfig(), bool *changed = nullptr);
// Declaration extraction from Pass.h and OpState; external implementations omitted.
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
namespace func {
class FuncOp {
public:
  MLIRContext *getContext();
  operator Operation *() const;
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
// TypeID.h public declaration only; storage/static-assert implementation omitted.
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CLASS_NAME) \
  static ::mlir::TypeID resolveTypeID();
#endif
