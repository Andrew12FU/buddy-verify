//===- ConvertMemcpyToGPU.cpp ---------------------------------------------===//
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
// This file implements the pass that converts memcpy to gpu operations.
//
// NOTE: this copy is syntactically adapted for the Frama-Clang front-end
//       (0.0.20).  Four constructs are rewritten, semantics are unchanged:
//         * the anonymous namespace becomes a named namespace
//           `convert_memcpy_to_gpu_detail`;
//         * `if (auto x = expr())` becomes a declaration followed by `if (x)`;
//         * the three range-based for loops become indexed loops;
//         * the walk lambdas no longer capture class objects by reference; the
//           environment is captured as pointers to the locals.
//
//===---------------------------------------------------------------------===//

#include "mlir/Dialect/GPU/IR/GPUDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/IR/Visitors.h"
#include "mlir/Support/LLVM.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <mlir/Dialect/Affine/IR/AffineOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/Linalg/Transforms/Transforms.h>
#include <mlir/IR/Dialect.h>
#include <mlir/IR/Operation.h>
#include <mlir/IR/TypeUtilities.h>
#include <mlir/IR/Value.h>
#include <mlir/Pass/Pass.h>

#include <vector>

using namespace mlir;
using namespace vector;

//===----------------------------------------------------------------------===//
// ConvertMemcpyToGPUPass
//===----------------------------------------------------------------------===//

namespace convert_memcpy_to_gpu_detail {

class ConvertMemcpyToGPUPass
    : public PassWrapper<ConvertMemcpyToGPUPass, OperationPass<func::FuncOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ConvertMemcpyToGPUPass)
  StringRef getArgument() const final { return "convert-memcpy-to-gpu"; }
  StringRef getDescription() const final {
    return "Convert memref opertaions to gpu operations.";
  }
  ConvertMemcpyToGPUPass() = default;
  ConvertMemcpyToGPUPass(const ConvertMemcpyToGPUPass &) {}

  Option<bool> processArgs{
      *this, "process-args",
      llvm::cl::desc("Whether the pass processes the input args."),
      llvm::cl::init(true)};

  void runOnOperation() override;

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<gpu::GPUDialect, memref::MemRefDialect>();
  }
};

MemRefType stripMemRefLayout(const MemRefType &base) {
  return MemRefType::get(base.getShape(), base.getElementType(), AffineMap(),
                         base.getMemorySpace());
}

void ConvertMemcpyToGPUPass::runOnOperation() {
  auto funcOp = getOperation();

  if (funcOp.isDeclaration() || funcOp.isExternal())
    return;

  // Make sure the gpu function is already outlined.
  funcOp->walk<WalkOrder::PreOrder>([](Operation *nestedOp) {
    gpu::LaunchOp gpuLaunchOp = dyn_cast<gpu::LaunchOp>(nestedOp);
    if (gpuLaunchOp) {
      nestedOp->emitOpError("The gpu function should be outlined.");
    }
    return WalkResult::advance();
  });

  std::vector<Value> unDeallocatedValue;
  OpBuilder builder(funcOp->getContext());

  // Frama-Clang cannot capture class-typed locals by reference, so the walk
  // body reaches the environment through pointers captured by value.
  OpBuilder *builderPtr = &builder;
  std::vector<Value> *unDeallocatedValuePtr = &unDeallocatedValue;
  func::FuncOp *funcOpPtr = &funcOp;

  // Copy all function arguments to gpu, needs deallocation
  if (processArgs) {
    builder.setInsertionPointToStart(&(funcOp.getBody().front()));
    unsigned numArgs = funcOp.getNumArguments();
    for (unsigned i = 0; i < numArgs; ++i) {
      BlockArgument arg = funcOp.getArgument(i);
      // Create a gpu.alloc op, then copy memory to it
      // TODO: Move this out of operation, make the copy process async
      auto memrefType = dyn_cast<MemRefType>(arg.getType());

      auto gpuAllocOp = gpu::AllocOp::create(
          builder, builder.getUnknownLoc(),
          TypeRange({stripMemRefLayout(memrefType)}), ValueRange({}));
      unDeallocatedValue.push_back(gpuAllocOp->getResult(0));
      auto gpuMemcpyOp =
          gpu::MemcpyOp::create(builder, gpuAllocOp.getLoc(), TypeRange(),
                                ValueRange(), gpuAllocOp->getResult(0), arg);
      arg.replaceAllUsesExcept(gpuAllocOp->getResult(0), gpuMemcpyOp);
    }
  }

  funcOp->walk<WalkOrder::PreOrder>(
      [builderPtr, unDeallocatedValuePtr, funcOpPtr](Operation *nestedOp) {
        // Frama-Clang has no if-with-initializer: hoist the casts.
        memref::AllocOp allocOp = dyn_cast<memref::AllocOp>(nestedOp);
        memref::CopyOp copyOp = dyn_cast<memref::CopyOp>(nestedOp);
        memref::GetGlobalOp getGlobalOp =
            dyn_cast<memref::GetGlobalOp>(nestedOp);
        func::ReturnOp returnOp = dyn_cast<func::ReturnOp>(nestedOp);

        // Replace all allocations with GPU.alloc
        if (allocOp) {
          // Rewrite this allocOp to gpu.alloc, change for all users
          builderPtr->setInsertionPointAfter(allocOp);
          auto result = allocOp->getResult(0);
          auto memrefType = dyn_cast<MemRefType>(result.getType());
          auto memorySpace = memrefType.getMemorySpace();

          // Filter operations.
          IntegerAttr intMemorySpace = llvm::dyn_cast<IntegerAttr>(memorySpace);
          gpu::AddressSpaceAttr gpuMemorySpace =
              llvm::dyn_cast<gpu::AddressSpaceAttr>(memorySpace);
          if (memorySpace) {
            if (intMemorySpace) {
              if (intMemorySpace.getInt() != 0) {
                return WalkResult::advance();
              }
            } else if (gpuMemorySpace) {
              if (gpuMemorySpace.getValue() != gpu::AddressSpace::Global) {
                return WalkResult::advance();
              }
            } else
              return WalkResult::advance();
          }

          auto gpuAllocOp = gpu::AllocOp::create(
              *builderPtr, allocOp->getLoc(),
              TypeRange({stripMemRefLayout(memrefType)}), ValueRange({}));

          user_range users = result.getUsers();
                    user_range::iterator userIt = users.begin();
                    user_range::iterator userItEnd = users.end();
           for (; userIt != userItEnd; ++userIt) {
            Operation *user = *userIt;
            memref::DeallocOp deallocOp = dyn_cast<memref::DeallocOp>(user);
            if (deallocOp) {
              builderPtr->setInsertionPointAfter(deallocOp);
              gpu::DeallocOp::create(*builderPtr, deallocOp->getLoc(),
                                     TypeRange(), ValueRange(),
                                     gpuAllocOp->getResult(0));
              deallocOp->erase();
            } else {
              llvm::MutableArrayRef<OpOperand> operands = user->getOpOperands();
              for (size_t operandIndex = 0; operandIndex < operands.size();
                   ++operandIndex) {
                OpOperand &opOperand = operands[operandIndex];
                if (opOperand.is(result)) {
                  opOperand.set(gpuAllocOp->getResult(0));
                }
              }
            }
          }
          allocOp->erase();
        }
        // Replace all memory.copy operations with gpu.memcpy
        else if (copyOp) {
          auto src = copyOp->getOperand(0);
          auto dst = copyOp->getOperand(1);
          // Notice: GPU.memcpy has a different src dst order
          builderPtr->setInsertionPointAfter(copyOp);
          auto gpuMemcpyOp = gpu::MemcpyOp::create(
              *builderPtr, copyOp->getLoc(), TypeRange(), ValueRange(), dst, src);
          src.replaceAllUsesWith(gpuMemcpyOp->getResult(1));
          dst.replaceAllUsesWith(gpuMemcpyOp->getResult(0));
          copyOp->erase();
        }
        // Allocate space on GPU and copy global memrefs to GPU, needs
        // deallocation
        else if (getGlobalOp) {
          builderPtr->setInsertionPointAfter(getGlobalOp);
          auto result = getGlobalOp->getResult(0);
          auto memrefType = dyn_cast<MemRefType>(result.getType());
          auto gpuAllocOp = gpu::AllocOp::create(
              *builderPtr, getGlobalOp->getLoc(),
              TypeRange({stripMemRefLayout(memrefType)}), ValueRange({}));
          unDeallocatedValuePtr->push_back(gpuAllocOp->getResult(0));

          auto src = result;
          auto dst = gpuAllocOp->getResult(0);
          auto gpuMemcpyOp = gpu::MemcpyOp::create(
              *builderPtr, gpuAllocOp.getLoc(), TypeRange(), ValueRange(), dst,
              src);
          src.replaceAllUsesExcept(dst, gpuMemcpyOp);
        }
        // Copy data back to CPU, deallocate GPU, then return
        else if (returnOp) {
          builderPtr->setInsertionPoint(returnOp);
          llvm::ArrayRef<Type> resultTypes =
              funcOpPtr->getFunctionType().getResults();
          llvm::SmallVector<Type> outputTypes;
          for (const Type *typeIt = resultTypes.begin();
               typeIt != resultTypes.end(); ++typeIt)
            outputTypes.push_back(*typeIt);
          for (unsigned i = 0; i < returnOp->getNumOperands(); ++i) {
            auto val = returnOp->getOperand(i);
            auto memrefType = dyn_cast<MemRefType>(val.getType());
            if (memrefType) {
              auto identityMemrefType = stripMemRefLayout(memrefType);
              auto allocOp = memref::AllocOp::create(
                  *builderPtr, returnOp->getLoc(), identityMemrefType);
              gpu::MemcpyOp::create(*builderPtr, allocOp.getLoc(), TypeRange(),
                                    ValueRange(), allocOp->getResult(0), val);
              // FIXME: may be leak memory
              // auto gpuDeallocOp = builder.create<gpu::DeallocOp>(
              //     gpuMemcpyOp->getLoc(), TypeRange(), ValueRange(), val);
              outputTypes[i] = identityMemrefType;
              returnOp->setOperand(i, allocOp->getResult(0));
            }
          }
          for (size_t valueIndex = 0;
               valueIndex < unDeallocatedValuePtr->size(); ++valueIndex) {
            Value value = (*unDeallocatedValuePtr)[valueIndex];
            gpu::DeallocOp::create(*builderPtr, returnOp->getLoc(), TypeRange(),
                                   ValueRange(), value);
          }
          funcOpPtr->setType(builderPtr->getFunctionType(
              funcOpPtr->getArgumentTypes(), outputTypes));
        }
        return WalkResult::advance();
      });
}
} // namespace convert_memcpy_to_gpu_detail

namespace mlir {
namespace buddy {
void registerConvertMemcpyToGPUPass() {
  PassRegistration<convert_memcpy_to_gpu_detail::ConvertMemcpyToGPUPass>();
}
} // namespace buddy
} // namespace mlir
