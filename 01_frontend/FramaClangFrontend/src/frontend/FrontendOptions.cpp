// SPDX-License-Identifier: Apache-2.0
#include "frontend/FrontendOptions.h"

namespace buddy::framac {
bool FrontendOptions::enabled(llvm::StringRef Name) const {
  return Rules.count(Name.str());
}

bool isSupportedProfile(llvm::StringRef Name) {
  return Name == "reshape" || Name == "matmul-scalar" || Name == "lower-ime" ||
         Name == "silu-fusion" || Name == "assume-tight" ||
         Name == "eliminate-copy" || Name == "gpu-memcpy" ||
         Name == "depthwise-conv" || Name == "builtin-transpose";
}

bool isKnownRule(llvm::StringRef Name) {
  return Name == "namespace" || Name == "while" || Name == "capture" ||
         Name == "guard" || Name == "if-condition" || Name == "value-range" ||
         Name == "const-local" || Name == "init-list" || Name == "range-for" ||
         Name == "constructor";
}

bool isInterfaceInclude(llvm::StringRef Profile, llvm::StringRef Header) {
  static const std::set<std::string> Common{
      "mlir/Dialect/Func/IR/FuncOps.h", "mlir/Dialect/Tosa/IR/TosaOps.h",
      "mlir/IR/PatternMatch.h", "mlir/Pass/Pass.h",
      "mlir/Transforms/GreedyPatternRewriteDriver.h"};
  static const std::set<std::string> MatmulExtra{"mlir/IR/Builders.h",
                                                 "mlir/IR/BuiltinAttributes.h",
                                                 "mlir/IR/BuiltinTypes.h"};
  static const std::set<std::string> IME{
      "mlir/Conversion/LLVMCommon/ConversionTarget.h",
      "mlir/Conversion/LLVMCommon/TypeConverter.h",
      "mlir/Dialect/Arith/IR/Arith.h",
      "mlir/Dialect/Func/IR/FuncOps.h",
      "mlir/Dialect/LLVMIR/LLVMDialect.h",
      "mlir/Dialect/MemRef/IR/MemRef.h",
      "mlir/Pass/Pass.h",
      "Dialect/IME/IMEDialect.h",
      "Dialect/IME/IMEOps.h",
      "Dialect/IME/Transform.h"};
  static const std::set<std::string> SiLUExtra{
      "mlir/Dialect/Arith/IR/Arith.h",
      "mlir/Dialect/Linalg/IR/Linalg.h",
      "mlir/Dialect/Math/IR/Math.h",
      "mlir/Dialect/Tensor/IR/Tensor.h",
      "mlir/IR/Builders.h",
      "mlir/IR/BuiltinTypes.h"};
  if (Profile == "lower-ime")
    return IME.count(Header.str());
  // 白名单只决定输出补丁，不改变 Clang 此轮实际读取的真实头文件。
  return Common.count(Header.str()) ||
         (Profile == "matmul-scalar" && MatmulExtra.count(Header.str())) ||
         (Profile == "silu-fusion" && SiLUExtra.count(Header.str()));
}
} // namespace buddy::framac
