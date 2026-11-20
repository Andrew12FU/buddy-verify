// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_TRANSFORMS_DIALECTCONVERSION_H
#define BUDDY_MINI_MLIR_TRANSFORMS_DIALECTCONVERSION_H
#include "mlir/Rewrite/FrozenRewritePatternSet.h"

namespace llvm {
template <typename T, typename Enable = void> struct DenseMapInfo;
template <typename ValueT, typename ValueInfoT = DenseMapInfo<ValueT>>
class DenseSet;
} // namespace llvm
namespace mlir {
class TypeConverter {
public:
  using Base = TypeConverter;
  virtual ~TypeConverter() = default;
  TypeConverter() = default;
  TypeConverter(const TypeConverter &other);
  TypeConverter &operator=(const TypeConverter &other);
};
class ConversionPatternRewriter final : public PatternRewriter {
public: using PatternRewriter::PatternRewriter;
};
class ConversionPattern : public RewritePattern {
public:
 using OpAdaptor=ArrayRef<Value>; using OneToNOpAdaptor=ArrayRef<ValueRange>;
 virtual LogicalResult matchAndRewrite(Operation *,ArrayRef<Value>,ConversionPatternRewriter &) const;
 virtual LogicalResult matchAndRewrite(Operation *,ArrayRef<ValueRange>,ConversionPatternRewriter &) const;
 LogicalResult matchAndRewrite(Operation *,PatternRewriter &) const final;
protected:
 using RewritePattern::RewritePattern;
 const TypeConverter *typeConverter = nullptr;
};
class ConversionTarget {
public:
  ConversionTarget(MLIRContext &ctx);
  template<typename... Names> void addLegalDialect(StringRef name,Names... names);
  template<typename... Args> void addLegalDialect();
  template<typename OpT> void addLegalOp();
  template<typename OpT,typename OpT2,typename... OpTs> void addLegalOp();
  virtual ~ConversionTarget() = default;
};
class Diagnostic;
enum class DialectConversionFoldingMode {
  Never,
  BeforePatterns,
  AfterPatterns
};
struct ConversionConfig {
  llvm::function_ref<void(Diagnostic &)> notifyCallback = nullptr;
  llvm::DenseSet<Operation *> *unlegalizedOps = nullptr;
  llvm::DenseSet<Operation *> *legalizableOps = nullptr;
  RewriterBase::Listener *listener = nullptr;
  bool buildMaterializations = true;
  bool allowPatternRollback = true;
  DialectConversionFoldingMode foldingMode =
      DialectConversionFoldingMode::BeforePatterns;
  bool attachDebugMaterializationKind = false;
};
LogicalResult
applyPartialConversion(ArrayRef<Operation *> ops,
                       const ConversionTarget &target,
                       const FrozenRewritePatternSet &patterns,
                       ConversionConfig config = ConversionConfig());
LogicalResult
applyPartialConversion(Operation *op, const ConversionTarget &target,
                       const FrozenRewritePatternSet &patterns,
                       ConversionConfig config = ConversionConfig());
} // namespace mlir
#endif
