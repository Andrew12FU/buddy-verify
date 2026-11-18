//====- LowerIMEPass.cpp - IME Dialect Lowering Pass  ---------------------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
//
// This file defines IME dialect lowering pass.
//
//===----------------------------------------------------------------------===//



#include "mini_mlir.h"

using namespace mlir;
using namespace buddy::ime;

namespace LowerIMEPass_anonymousNamespace_01 {
class LowerIMEToLLVMPass
    : public PassWrapper<LowerIMEToLLVMPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerIMEToLLVMPass)
  StringRef getArgument() const final { return "lower-ime"; }
  StringRef getDescription() const final {
    return "IME dialect lowering pass.";
  }
  LowerIMEToLLVMPass() = default;
  LowerIMEToLLVMPass(const LowerIMEToLLVMPass &) {}

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<LLVM::LLVMDialect>();
    registry.insert<IMEDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<memref::MemRefDialect>();
  }

  void runOnOperation() override;
};
} // namespace

void LowerIMEPass_anonymousNamespace_01::LowerIMEToLLVMPass::runOnOperation() {
  MLIRContext *context = &getContext();
  ModuleOp module = getOperation();

  LLVMTypeConverter converter(context);
  RewritePatternSet patterns(context);
  LLVMConversionTarget target(*context);

  configureIMELegalizeForExportTarget(target);
  populateIMELegalizeForLLVMExportPatterns(converter, patterns);

  if (failed(applyPartialConversion(module, target, std::move(patterns))))
    signalPassFailure();
}

namespace mlir {
namespace buddy {
void registerLowerIMEPass() { PassRegistration<LowerIMEPass_anonymousNamespace_01::LowerIMEToLLVMPass>(); }
} // namespace buddy
} // namespace mlir
