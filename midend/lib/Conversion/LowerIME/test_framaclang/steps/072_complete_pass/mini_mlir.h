#ifndef LOWER_IME_MINI_MLIR_H
#define LOWER_IME_MINI_MLIR_H
// Static declarations for conversion only; no semantic contracts.
namespace mlir {}
namespace buddy { namespace ime {} }
namespace llvm { class StringRef { public: constexpr StringRef(const char *Str); }; }
namespace mlir { using StringRef = llvm::StringRef; }
namespace std {
template <typename T> struct default_delete;
template <typename T, typename Deleter = default_delete<T>> class unique_ptr;
}
namespace mlir {
class TypeID {};
class MLIRContext;
class Operation;
class DialectRegistry;
class RegisteredOperationName;
class Pass {
public:
 virtual ~Pass() = default;
 virtual StringRef getName() const = 0;
 virtual StringRef getArgument() const;
 virtual StringRef getDescription() const;
 virtual void getDependentDialects(DialectRegistry &registry) const;
protected:
 Pass(const Pass &other);
 virtual bool canScheduleOn(RegisteredOperationName opName) const = 0;
 virtual bool canScheduleOn(Operation *op) const;
 virtual std::unique_ptr<Pass> clonePass() const = 0;
 MLIRContext &getContext();
 virtual void runOnOperation() = 0;
 void signalPassFailure();
};
class ModuleOp { public: operator Operation *() const; };
}
namespace mlir {
template <typename OpT = void> class OperationPass : public Pass {
public:
 ~OperationPass() override = default;
protected:
 OperationPass(TypeID passID);
 OperationPass(const OperationPass &) = default;
 OperationPass &operator=(const OperationPass &) = delete;
 OperationPass(OperationPass &&) = delete;
 OperationPass &operator=(OperationPass &&) = delete;
 bool canScheduleOn(RegisteredOperationName opName) const final;
 OpT getOperation();
};
}
namespace mlir {
template <typename PassT, typename BaseT> class PassWrapper : public BaseT {
public:
 ~PassWrapper() override = default;
protected:
 PassWrapper();
 PassWrapper(const PassWrapper &) = default;
 PassWrapper &operator=(const PassWrapper &) = delete;
 PassWrapper(PassWrapper &&) = delete;
 PassWrapper &operator=(PassWrapper &&) = delete;
 StringRef getName() const override;
 std::unique_ptr<Pass> clonePass() const override;
};
}
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CLASS_NAME) \
 static ::mlir::TypeID resolveTypeID();
namespace mlir {
class DialectRegistry {
public:
 explicit DialectRegistry();
 DialectRegistry(const DialectRegistry &) = delete;
 DialectRegistry &operator=(const DialectRegistry &) = delete;
 DialectRegistry(DialectRegistry &&) = default;
 DialectRegistry &operator=(DialectRegistry &&) = default;
 template <typename ConcreteDialect> void insert();
 template <typename ConcreteDialect, typename OtherDialect, typename... MoreDialects> void insert();
};
}
namespace mlir { namespace LLVM { class LLVMDialect; } }
namespace buddy { namespace ime { class IMEDialect; } }
namespace mlir { namespace arith { class ArithDialect; } }
namespace mlir { namespace memref { class MemRefDialect; } }
namespace mlir {
class TypeConverter {
public:
 using Base = TypeConverter;
 virtual ~TypeConverter() = default;
 TypeConverter() = default;
 TypeConverter(const TypeConverter &other);
 TypeConverter &operator=(const TypeConverter &other);
};
}
namespace mlir {
class DataLayoutAnalysis;
class LowerToLLVMOptions;
class LLVMTypeConverter : public TypeConverter {
public:
 LLVMTypeConverter(MLIRContext *ctx, const DataLayoutAnalysis *analysis = nullptr);
 LLVMTypeConverter(MLIRContext *ctx, const LowerToLLVMOptions &options,
                   const DataLayoutAnalysis *analysis = nullptr);
};
}
namespace mlir {
class RewritePatternSet {
public:
 RewritePatternSet(MLIRContext *context);
};
}
namespace mlir {
class ConversionTarget {
public:
 ConversionTarget(MLIRContext &ctx);
 virtual ~ConversionTarget() = default;
};
}
namespace mlir {
class LLVMConversionTarget : public ConversionTarget {
public:
 explicit LLVMConversionTarget(MLIRContext &ctx);
};
}
namespace mlir {
void configureIMELegalizeForExportTarget(LLVMConversionTarget &target);
}
namespace mlir {
void populateIMELegalizeForLLVMExportPatterns(LLVMTypeConverter &converter,
                                           RewritePatternSet &patterns);
}
typedef unsigned long size_t;
namespace std {
template <bool B, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true,T> { using type = T; };
template <bool B, typename T = void> using enable_if_t = typename enable_if<B,T>::type;
template <typename T> T &&declval() noexcept;
template <typename From, typename To> struct is_convertible {
 static constexpr bool value = __is_convertible(From, To);
};
template <typename T> struct is_integral { static constexpr bool value = __is_integral(T); };
template <bool B, typename T, typename F> struct conditional { using type = T; };
template <typename T, typename F> struct conditional<false,T,F> { using type = F; };
template <typename... B> struct conjunction;
template <> struct conjunction<> { static constexpr bool value = true; };
template <typename B> struct conjunction<B> : B {};
template <typename B1, typename... Bn> struct conjunction<B1,Bn...>
 : conditional<bool(B1::value),conjunction<Bn...>,B1>::type {};
}
namespace std {
template <typename T> class initializer_list {
public:
 typedef const T *const_iterator;
 typedef size_t size_type;
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
template <typename T> class ArrayRef {
 const T *Data = nullptr;
 size_t Length = 0;
public:
 ArrayRef() = default;
 ArrayRef(const T &OneElt);
 constexpr ArrayRef(const T *data, size_t length);
 constexpr ArrayRef(const T *begin, const T *end);
 template <typename C, typename = std::enable_if_t<std::conjunction<
   std::is_convertible<decltype(std::declval<const C &>().data()) *, const T *const *>,
   std::is_integral<decltype(std::declval<const C &>().size())>>::value, void>>
 constexpr ArrayRef(const C &V);
 template <size_t N> constexpr ArrayRef(const T (&Arr)[N]);
 constexpr ArrayRef(std::initializer_list<T> Vec);
};
}
namespace mlir { using llvm::ArrayRef; }
namespace std {
template <typename T> struct char_traits;
template <typename T> class allocator;
namespace __cxx11 { template <typename CharT, typename Traits, typename Allocator> class basic_string; }
using string = __cxx11::basic_string<char, char_traits<char>, allocator<char>>;
template <typename T> struct remove_reference { using type = T; };
template <typename T> struct remove_reference<T &> { using type = T; };
template <typename T> struct remove_reference<T &&> { using type = T; };
template <typename T> constexpr typename remove_reference<T>::type &&move(T &&value) noexcept;
}
namespace mlir {
class FrozenRewritePatternSet {
public:
 FrozenRewritePatternSet();
 FrozenRewritePatternSet(FrozenRewritePatternSet &&patterns) = default;
 FrozenRewritePatternSet(const FrozenRewritePatternSet &patterns) = default;
 FrozenRewritePatternSet &operator=(const FrozenRewritePatternSet &patterns) = default;
 FrozenRewritePatternSet &operator=(FrozenRewritePatternSet &&patterns) = default;
 ~FrozenRewritePatternSet();
 FrozenRewritePatternSet(RewritePatternSet &&patterns,
  ArrayRef<std::string> disabledPatternLabels = {}, ArrayRef<std::string> enabledPatternLabels = {});
};
}
typedef long intptr_t;
namespace std { using nullptr_t = decltype(nullptr); }
namespace llvm {
template <typename T, typename Enable = void> struct DenseMapInfo;
template <typename ValueT, typename ValueInfoT = DenseMapInfo<ValueT>> class DenseSet;
template <typename Fn> class function_ref;
template <typename Ret, typename... Params> class function_ref<Ret(Params...)> {
 Ret (*callback)(intptr_t callable, Params... params) = nullptr;
 intptr_t callable;
public:
 function_ref() = default;
 function_ref(std::nullptr_t);
};
}
namespace mlir {
class RewriterBase { public: struct Listener; };
class Diagnostic;
enum class DialectConversionFoldingMode { Never, BeforePatterns, AfterPatterns };
struct ConversionConfig {
 llvm::function_ref<void(Diagnostic &)> notifyCallback = nullptr;
 llvm::DenseSet<Operation *> *unlegalizedOps = nullptr;
 llvm::DenseSet<Operation *> *legalizableOps = nullptr;
 RewriterBase::Listener *listener = nullptr;
 bool buildMaterializations = true;
 bool allowPatternRollback = true;
 DialectConversionFoldingMode foldingMode = DialectConversionFoldingMode::BeforePatterns;
 bool attachDebugMaterializationKind = false;
};
}
namespace llvm {
struct [[nodiscard]] LogicalResult {
public:
 static LogicalResult success(bool IsSuccess = true);
 static LogicalResult failure(bool IsFailure = true);
 constexpr bool succeeded() const;
 constexpr bool failed() const;
private:
 LogicalResult(bool IsSuccess);
 bool IsSuccess;
};
bool failed(LogicalResult Result);
}
namespace mlir { using LogicalResult = llvm::LogicalResult; using llvm::failed; }
namespace mlir {
LogicalResult applyPartialConversion(ArrayRef<Operation *> ops, const ConversionTarget &target,
 const FrozenRewritePatternSet &patterns, ConversionConfig config = ConversionConfig());
LogicalResult applyPartialConversion(Operation *op, const ConversionTarget &target,
 const FrozenRewritePatternSet &patterns, ConversionConfig config = ConversionConfig());
}
namespace std { template <typename Fn> class function; }
namespace mlir {
using PassAllocatorFunction = std::function<std::unique_ptr<Pass>()>;
template <typename ConcretePass> struct PassRegistration {
 PassRegistration(const PassAllocatorFunction &constructor);
 PassRegistration();
};
}
#endif
