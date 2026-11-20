//====- DepthwiseConvNhwcHwc.cpp
//--------------------------------------------------===//
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
// This file implements the DepthwiseConvNhwcHwc optimize.
//
// NOTE: syntactically adapted for Frama-Clang 0.0.20; the lambda structure
//       is preserved.  Only two capture rules are applied:
//         * a nested lambda may only capture variables of its immediately
//           enclosing lambda, so every level copies the environment into a
//           local (e1/e2/e3) before creating the next builder;
//         * class-typed objects are captured by value (never by reference),
//           which is what the front-end can represent.
//
//===----------------------------------------------------------------------===//

#include <mlir/Dialect/Affine/IR/AffineOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/Linalg/Transforms/Transforms.h>
#include <mlir/IR/IntegerSet.h>
#include <mlir/Pass/Pass.h>

using namespace mlir;
using namespace vector;

//===----------------------------------------------------------------------===//
// Rewrite Pattern
//===----------------------------------------------------------------------===//

namespace depthwise_conv_detail {

/* Everything the nested builders capture from `matchAndRewrite`.  Frama-Clang
 * only lets a nested lambda capture variables declared in its immediately
 * enclosing lambda, so each level copies this struct into a local (`e1`, `e2`,
 * `e3`) before creating the next builder. */
struct DepthwiseEnv {
  Value c0;
  Value c1;
  Value vecSizeValue;
  AffineExpr d0;
  AffineExpr d1;

  Value input;
  Value filter;
  Value output;

  Value FH;
  Value FW;
  Value applyOC;
  Value tailLength;
  Value maskVector;
  Value zeroElementTypeVec;

  VectorType vecTy;
  Type elemTy;

  int strHeight;
  int strWidth;
  int dilHeight;
  int dilWidth;
};

class DepthwiseConv2DNhwcHwcOptimizePattern : public ConversionPattern {
public:
  explicit DepthwiseConv2DNhwcHwcOptimizePattern(MLIRContext *context,
                                                 int64_t vecSizeParam)
      : ConversionPattern(linalg::DepthwiseConv2DNhwcHwcOp::getOperationName(),
                          1, context) {
    vecSize = vecSizeParam;
  }

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> /*operands*/,
                  ConversionPatternRewriter &rewriter) const override {
    auto convOp = dyn_cast_or_null<mlir::linalg::DepthwiseConv2DNhwcHwcOp>(op);
    auto loc = op->getLoc();

    // Some constant we need.
    Value c0Init =
        arith::ConstantOp::create(rewriter, loc, rewriter.getIndexAttr(0));
    const Value c0 = c0Init;
    Value c1Init =
        arith::ConstantOp::create(rewriter, loc, rewriter.getIndexAttr(1));
    const Value c1 = c1Init;

    Value vecSizeValueInit = arith::ConstantOp::create(
        rewriter, loc, rewriter.getIndexAttr(vecSize));
    const Value vecSizeValue = vecSizeValueInit;
    AffineExpr d0Init = rewriter.getAffineDimExpr(0);
    const AffineExpr d0 = d0Init;
    AffineExpr d1Init = rewriter.getAffineDimExpr(1);
    const AffineExpr d1 = d1Init;
    // TODO: remove s0?
    // const AffineExpr s0 = rewriter.getAffineSymbolExpr(0);

    Value input = op->getOperand(0);
    Value filter = op->getOperand(1);
    Value output = op->getOperand(2);

    int strHeight, strWidth, dilHeight, dilWidth;

    // Strides.
    if (!convOp.getStrides()) {
      strHeight = 1;
      strWidth = 1;
    } else {
      auto strideVals = convOp.getStrides().getValues<int64_t>();
      auto strideIt = strideVals.begin();
      strHeight = *strideIt;
      ++strideIt;
      strWidth = *strideIt;
    }

    // Dilations.
    if (!convOp.getDilations()) {
      dilHeight = 1;
      dilWidth = 1;
    } else {
      auto dilationVals = convOp.getDilations().getValues<int64_t>();
      auto dilationIt = dilationVals.begin();
      dilHeight = *dilationIt;
      ++dilationIt;
      dilWidth = *dilationIt;
    }

    ShapedType inputTy = mlir::cast<mlir::ShapedType>(input.getType());
    Type elemTy = inputTy.getElementType();
    VectorType vecTy = VectorType::get(llvm::ArrayRef<int64_t>(&vecSize, 1), elemTy);

    Value zeroElementTypeInit =
        arith::ConstantOp::create(rewriter, loc, rewriter.getZeroAttr(elemTy));
    const Value zeroElementType = zeroElementTypeInit;

    Value zeroElementTypeVec;
    if (isa<IntegerType>(elemTy)) {
      zeroElementTypeVec =
          vector::BroadcastOp::create(rewriter, loc, vecTy, zeroElementType);
    } else {
      zeroElementTypeVec =
          vector::BroadcastOp::create(rewriter, loc, vecTy, zeroElementType);
    }
    // Dims
    Value N = memref::DimOp::create(rewriter, loc, output, 0);  // N
    Value OH = memref::DimOp::create(rewriter, loc, output, 1); // OH
    Value OW = memref::DimOp::create(rewriter, loc, output, 2); // OW
    Value OC = memref::DimOp::create(rewriter, loc, output, 3); // OC/FC/IC

    Value applyOC = affine::AffineApplyOp::create(
        rewriter, loc, AffineMap::get(1, 0, d0.floorDiv(vecSize) * vecSize),
        OC);
    Value tailLength = affine::AffineApplyOp::create(
        rewriter, loc, AffineMap::get(1, 0, d0 % vecSize), ValueRange(OC));
    Value maskVector = vector::CreateMaskOp::create(
        rewriter, loc, VectorType::get(llvm::ArrayRef<int64_t>(&vecSize, 1), rewriter.getI1Type()),
        ValueRange(tailLength));

    Value FH = memref::DimOp::create(rewriter, loc, filter, 0); // FH
    Value FW = memref::DimOp::create(rewriter, loc, filter, 1); // FW

    // Fill the environment once; the lambdas below copy it per level.
    DepthwiseEnv env;
    env.c0 = c0;
    env.c1 = c1;
    env.vecSizeValue = vecSizeValue;
    env.d0 = d0;
    env.d1 = d1;
    env.input = input;
    env.filter = filter;
    env.output = output;
    env.FH = FH;
    env.FW = FW;
    env.applyOC = applyOC;
    env.tailLength = tailLength;
    env.maskVector = maskVector;
    env.zeroElementTypeVec = zeroElementTypeVec;
    env.vecTy = vecTy;
    env.elemTy = elemTy;
    env.strHeight = strHeight;
    env.strWidth = strWidth;
    env.dilHeight = dilHeight;
    env.dilWidth = dilWidth;

    SmallVector<OpFoldResult, 3> lowerBounds;
    lowerBounds.push_back(N);
    lowerBounds.push_back(OH);
    lowerBounds.push_back(OW);

    // clang format off
    //  Step 1: Create outer most loops.
    // Create the scf::ForallOp operation For N,OH,OW
    scf::ForallOp::create(
        rewriter, loc, lowerBounds, ValueRange(),
        std::nullopt, // No mapping specified in this example
        [env, loc](OpBuilder &nestedBuilder, Location nestedLoc,
              ValueRange loopIndices) {
          Value ivN = loopIndices[0];  // Index for the first dimension N
          Value ivOH = loopIndices[1]; // Index for the second dimension OH
          Value ivOW = loopIndices[2]; // Index for the third dimension OW
          DepthwiseEnv e1 = env;
          // OC
          scf::ForOp::create(
              nestedBuilder, nestedLoc, e1.c0, e1.applyOC, e1.vecSizeValue,
              ValueRange(),
              [e1, ivN, ivOH, ivOW](OpBuilder &builder, Location loc,
                                    Value ivOC, ValueRange iargs) {
                Value tVec = vector::LoadOp::create(
                    builder, loc, e1.vecTy, e1.output,
                    ValueRange(ivN, ivOH, ivOW, ivOC));

                DepthwiseEnv e2 = e1;
                Value vN = ivN;
                Value vOH = ivOH;
                Value vOW = ivOW;

                // FH
                auto forOp = scf::ForOp::create(
                    builder, loc, e2.c0, e2.FH, e2.c1, ValueRange(tVec),
                    [e2, vN, vOH, vOW, ivOC](OpBuilder &builder, Location loc,
                                             Value ivFH, ValueRange iargs) {
                      Value rowInput = affine::AffineApplyOp::create(
                          builder, loc,
                          AffineMap::get(
                              2, 0, e2.d0 * e2.strHeight + e2.d1 * e2.dilHeight),
                          ValueRange(vOH, ivFH));
                      Value rowFilter = ivFH;

                      DepthwiseEnv e3 = e2;
                      Value wN = vN;
                      Value wOW = vOW;
                      Value wOC = ivOC;

                      // FW
                      auto forOp = scf::ForOp::create(
                          builder, loc, e3.c0, e3.FW, e3.c1,
                          ValueRange(iargs[0]),
                          [e3, wN, wOW, wOC, rowInput,
                           rowFilter](OpBuilder &builder, Location loc,
                                      Value ivFW, ValueRange iargs) {
                            Value columnInput = affine::AffineApplyOp::create(
                                builder, loc,
                                AffineMap::get(2, 0,
                                               e3.d0 * e3.strWidth +
                                                   e3.d1 * e3.dilWidth),
                                ValueRange(wOW, ivFW));
                            Value columnFilter = affine::AffineApplyOp::create(
                                builder, loc, AffineMap::get(1, 0, e3.d0),
                                ivFW);
                            Value iVec = vector::LoadOp::create(
                                builder, loc, e3.vecTy, e3.input,
                                ValueRange(wN, rowInput, columnInput, wOC));
                            Value fVec = vector::LoadOp::create(
                                builder, loc, e3.vecTy, e3.filter,
                                ValueRange(rowFilter, columnFilter, wOC));
                            Value tVecNext;
                            if (isa<IntegerType>(e3.elemTy)) {
                              Value mulVec = arith::MulIOp::create(
                                  builder, loc, iVec, fVec);
                              tVecNext = arith::AddIOp::create(
                                  builder, loc, mulVec, iargs[0]);
                            } else {
                              tVecNext = vector::FMAOp::create(
                                  builder, loc, e3.vecTy, iVec, fVec, iargs[0]);
                            }

                            scf::YieldOp::create(builder, loc,
                                                 ValueRange(tVecNext));
                          });
                      scf::YieldOp::create(builder, loc,
                                           ValueRange(forOp->getResult(0)));
                    });
                vector::StoreOp::create(builder, loc, forOp->getResult(0),
                                        e1.output,
                                        ValueRange(ivN, ivOH, ivOW, ivOC));

                scf::YieldOp::create(builder, loc, ValueRange());
              });

          // applyOC
          Value condition = arith::CmpIOp::create(
              nestedBuilder, loc, arith::CmpIPredicate::sgt, e1.tailLength,
              e1.c0);
          scf::IfOp::create(
              nestedBuilder, loc, condition,
              [e1, ivN, ivOH, ivOW](OpBuilder &builder, Location loc) {
                Value tVec = vector::MaskedLoadOp::create(
                    builder, loc, e1.vecTy, e1.output,
                    ValueRange(ivN, ivOH, ivOW, e1.applyOC), e1.maskVector,
                    e1.zeroElementTypeVec);

                DepthwiseEnv e2 = e1;
                Value tN = ivN;
                Value tOH = ivOH;
                Value tOW = ivOW;

                // FH
                auto forOp = scf::ForOp::create(
                    builder, loc, e2.c0, e2.FH, e2.c1, ValueRange(tVec),
                    [e2, tN, tOH, tOW](OpBuilder &builder, Location loc,
                                       Value ivFH, ValueRange iargs) {
                      Value rowInput = affine::AffineApplyOp::create(
                          builder, loc,
                          AffineMap::get(
                              2, 0, e2.d0 * e2.strHeight + e2.d1 * e2.dilHeight),
                          ValueRange(tOH, ivFH));
                      Value rowFilter = ivFH;

                      DepthwiseEnv e3 = e2;
                      Value uN = tN;
                      Value uOW = tOW;

                      // FW
                      auto forOp = scf::ForOp::create(
                          builder, loc, e3.c0, e3.FW, e3.c1,
                          ValueRange(iargs[0]),
                          [e3, uN, uOW, rowInput,
                           rowFilter](OpBuilder &builder, Location loc,
                                      Value ivFW, ValueRange iargs) {
                            Value columnInput = affine::AffineApplyOp::create(
                                builder, loc,
                                AffineMap::get(2, 0,
                                               e3.d0 * e3.strWidth +
                                                   e3.d1 * e3.dilWidth),
                                ValueRange(uOW, ivFW));
                            Value columnFilter = affine::AffineApplyOp::create(
                                builder, loc, AffineMap::get(1, 0, e3.d0),
                                ivFW);
                            Value iVec = vector::MaskedLoadOp::create(
                                builder, loc, e3.vecTy, e3.input,
                                ValueRange(uN, rowInput, columnInput,
                                           e3.applyOC),
                                e3.maskVector, e3.zeroElementTypeVec);
                            Value fVec = vector::MaskedLoadOp::create(
                                builder, loc, e3.vecTy, e3.filter,
                                ValueRange(rowFilter, columnFilter,
                                           e3.applyOC),
                                e3.maskVector, e3.zeroElementTypeVec);
                            Value tVecNext;
                            if (isa<IntegerType>(e3.elemTy)) {
                              Value mulVec = arith::MulIOp::create(
                                  builder, loc, iVec, fVec);
                              tVecNext = arith::AddIOp::create(
                                  builder, loc, mulVec, iargs[0]);
                            } else {
                              tVecNext = vector::FMAOp::create(
                                  builder, loc, e3.vecTy, iVec, fVec, iargs[0]);
                            }

                            scf::YieldOp::create(builder, loc,
                                                 ValueRange(tVecNext));
                          });
                      scf::YieldOp::create(builder, loc,
                                           ValueRange(forOp->getResult(0)));
                    });
                vector::MaskedStoreOp::create(
                    builder, loc, e1.output,
                    ValueRange(ivN, ivOH, ivOW, e1.applyOC), e1.maskVector,
                    forOp->getResult(0));
                scf::YieldOp::create(builder, loc, ValueRange());
              });

          scf::InParallelOp::create(nestedBuilder, nestedLoc);
        });
    // clang format on

    rewriter.eraseOp(op);
    return success();
  }

private:
  int64_t vecSize;
};
} // namespace depthwise_conv_detail

//===----------------------------------------------------------------------===//
// DepthwiseConv2DNhwcHwcOptimizePass
//===----------------------------------------------------------------------===//

namespace depthwise_pass_detail {

class DepthwiseConv2DNhwcHwcOptimizePass
    : public PassWrapper<DepthwiseConv2DNhwcHwcOptimizePass,
                         OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      DepthwiseConv2DNhwcHwcOptimizePass)
  StringRef getArgument() const final {
    return "depthwise-conv-nhwc-hwc-optimize";
  }
  StringRef getDescription() const final {
    return "Depthwise Conv2d NHWC HWC optimize.";
  }
  DepthwiseConv2DNhwcHwcOptimizePass() = default;
  DepthwiseConv2DNhwcHwcOptimizePass(
      const DepthwiseConv2DNhwcHwcOptimizePass &) {}
  explicit DepthwiseConv2DNhwcHwcOptimizePass(int64_t vecSizeParam) {
    vecSize = vecSizeParam;
  }

  void runOnOperation() override;

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, scf::SCFDialect,
                    affine::AffineDialect, VectorDialect>();
  }

  Option<int64_t> vecSize{*this, "vec-size", llvm::cl::desc("Vector size."),
                          llvm::cl::init(16)};
};
} // namespace depthwise_pass_detail

void depthwise_pass_detail::DepthwiseConv2DNhwcHwcOptimizePass::
    runOnOperation() {
  MLIRContext *context = &getContext();
  ModuleOp module = getOperation();

  ConversionTarget target(*context);
  target
      .addLegalDialect<arith::ArithDialect, affine::AffineDialect,
                       scf::SCFDialect, memref::MemRefDialect, VectorDialect>();
  target.addLegalOp<ModuleOp, func::FuncOp, func::ReturnOp>();
  target.addLegalOp<linalg::FillOp>();

  RewritePatternSet patterns(context);
  patterns.add<depthwise_conv_detail::DepthwiseConv2DNhwcHwcOptimizePattern>(context, vecSize);

  if (failed(applyPartialConversion(module, target, std::move(patterns))))
    signalPassFailure();
}

namespace mlir {
namespace buddy {
void registerDepthwiseConv2DNhwcHwcOptimizePass() {
  PassRegistration<depthwise_pass_detail::DepthwiseConv2DNhwcHwcOptimizePass>();
}
} // namespace buddy
} // namespace mlir
