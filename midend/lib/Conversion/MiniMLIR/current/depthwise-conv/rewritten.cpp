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

namespace DepthwiseConvNhwcHwc_anonymousNamespace_01 {

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
    mlir::Value c0Storage = arith::ConstantOp::create(rewriter, loc, rewriter.getIndexAttr(0));
    const mlir::Value &c0 = c0Storage;
    mlir::Value c1Storage = arith::ConstantOp::create(rewriter, loc, rewriter.getIndexAttr(1));
    const mlir::Value &c1 = c1Storage;

    mlir::Value vecSizeValueStorage = arith::ConstantOp::create(
        rewriter, loc, rewriter.getIndexAttr(vecSize));
    const mlir::Value &vecSizeValue = vecSizeValueStorage;
    mlir::AffineExpr d0Storage = rewriter.getAffineDimExpr(0);
    const mlir::AffineExpr &d0 = d0Storage;
    mlir::AffineExpr d1Storage = rewriter.getAffineDimExpr(1);
    const mlir::AffineExpr &d1 = d1Storage;
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
      strHeight = convOp.getStrides().getValues<int64_t>()[0];
      strWidth = convOp.getStrides().getValues<int64_t>()
                     [convOp.getStrides().getValues<int64_t>().size() - 1];
    }

    // Dilations.
    if (!convOp.getDilations()) {
      dilHeight = 1;
      dilWidth = 1;
    } else {
      dilHeight = convOp.getDilations().getValues<int64_t>()[0];
      dilWidth = convOp.getDilations().getValues<int64_t>()
                     [convOp.getDilations().getValues<int64_t>().size() - 1];
    }

    ShapedType inputTy = mlir::cast<mlir::ShapedType>(input.getType());
    Type elemTy = inputTy.getElementType();
    VectorType vecTy = VectorType::get(vecSize, elemTy);

    mlir::Value zeroElementTypeStorage = arith::ConstantOp::create(rewriter, loc, rewriter.getZeroAttr(elemTy));
    const mlir::Value &zeroElementType = zeroElementTypeStorage;

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
    mlir::Value listValues[] = {OC};
    Value tailLength = affine::AffineApplyOp::create(
        rewriter, loc, AffineMap::get(1, 0, d0 % vecSize), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues, 1)));
    int64_t listValues1[] = {vecSize};
    mlir::Value listValues2[] = {tailLength};
    Value maskVector = vector::CreateMaskOp::create(
        rewriter, loc, VectorType::get(llvm::ArrayRef<int64_t>(listValues1, 1), rewriter.getI1Type()),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues2, 1)));

    Value FH = memref::DimOp::create(rewriter, loc, filter, 0); // FH
    Value FW = memref::DimOp::create(rewriter, loc, filter, 1); // FW

    // clang format off
    //  Step 1: Create outer most loops.
    // Create the scf::ForallOp operation For N,OH,OW
    mlir::OpFoldResult listValues3[] = {N, OH, OW};
    auto *c0Ptr = &c0;
    auto *applyOCPtr = &applyOC;
    auto *vecSizeValuePtr = &vecSizeValue;
    auto *vecTyPtr = &vecTy;
    auto *outputPtr = &output;
    auto *FHPtr = &FH;
    auto *c1Ptr = &c1;
    auto *d0Ptr = &d0;
    auto *strHeightPtr = &strHeight;
    auto *d1Ptr = &d1;
    auto *dilHeightPtr = &dilHeight;
    auto *FWPtr = &FW;
    auto *strWidthPtr = &strWidth;
    auto *dilWidthPtr = &dilWidth;
    auto *inputPtr = &input;
    auto *filterPtr = &filter;
    auto *elemTyPtr = &elemTy;
    auto *locPtr = &loc;
    auto *tailLengthPtr = &tailLength;
    auto *maskVectorPtr = &maskVector;
    auto *zeroElementTypeVecPtr = &zeroElementTypeVec;
    scf::ForallOp::create(
        rewriter, loc, llvm::SmallVector<mlir::OpFoldResult, 3>(listValues3, listValues3 + 3), ValueRange{},
        std::nullopt, // No mapping specified in this example
        [c0Ptr, applyOCPtr, vecSizeValuePtr, vecTyPtr, outputPtr, FHPtr, c1Ptr, d0Ptr, strHeightPtr, d1Ptr, dilHeightPtr, FWPtr, strWidthPtr, dilWidthPtr, inputPtr, filterPtr, elemTyPtr, locPtr, tailLengthPtr, maskVectorPtr, zeroElementTypeVecPtr](OpBuilder &nestedBuilder, Location nestedLoc,
            ValueRange loopIndices) {
          Value ivN = loopIndices[0];  // Index for the first dimension N
          Value ivOH = loopIndices[1]; // Index for the second dimension OH
          Value ivOW = loopIndices[2]; // Index for the third dimension OW
          // OC
          auto *ivNPtr = &ivN;
          auto *ivOHPtr = &ivOH;
          auto *ivOWPtr = &ivOW;
          auto *vecTyPtr1 = vecTyPtr;
          auto *outputPtr1 = outputPtr;
          auto *c0Ptr1 = c0Ptr;
          auto *FHPtr1 = FHPtr;
          auto *c1Ptr1 = c1Ptr;
          auto *d0Ptr1 = d0Ptr;
          auto *strHeightPtr1 = strHeightPtr;
          auto *d1Ptr1 = d1Ptr;
          auto *dilHeightPtr1 = dilHeightPtr;
          auto *FWPtr1 = FWPtr;
          auto *strWidthPtr1 = strWidthPtr;
          auto *dilWidthPtr1 = dilWidthPtr;
          auto *inputPtr1 = inputPtr;
          auto *filterPtr1 = filterPtr;
          auto *elemTyPtr1 = elemTyPtr;
          scf::ForOp::create(
              nestedBuilder, nestedLoc, (*c0Ptr), (*applyOCPtr), (*vecSizeValuePtr), ValueRange{},
              [ivNPtr, ivOHPtr, ivOWPtr, vecTyPtr1, outputPtr1, c0Ptr1, FHPtr1, c1Ptr1, d0Ptr1, strHeightPtr1, d1Ptr1, dilHeightPtr1, FWPtr1, strWidthPtr1, dilWidthPtr1, inputPtr1, filterPtr1, elemTyPtr1](OpBuilder &builder, Location loc, Value ivOC,
                  ValueRange iargs) {
                mlir::Value listValues4[] = {(*ivNPtr), (*ivOHPtr), (*ivOWPtr), ivOC};
                Value tVec =
                    vector::LoadOp::create(builder, loc, (*vecTyPtr1), (*outputPtr1),
                                           mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues4, 4)));

                // FH
                mlir::Value listValues5[] = {tVec};
                auto *ivOHPtr1 = ivOHPtr;
                auto *d0Ptr2 = d0Ptr1;
                auto *strHeightPtr2 = strHeightPtr1;
                auto *d1Ptr2 = d1Ptr1;
                auto *dilHeightPtr2 = dilHeightPtr1;
                auto *c0Ptr2 = c0Ptr1;
                auto *FWPtr2 = FWPtr1;
                auto *c1Ptr2 = c1Ptr1;
                auto *ivOWPtr1 = ivOWPtr;
                auto *strWidthPtr2 = strWidthPtr1;
                auto *dilWidthPtr2 = dilWidthPtr1;
                auto *ivNPtr1 = ivNPtr;
                auto *ivOCPtr = &ivOC;
                auto *vecTyPtr2 = vecTyPtr1;
                auto *inputPtr2 = inputPtr1;
                auto *filterPtr2 = filterPtr1;
                auto *elemTyPtr2 = elemTyPtr1;
                auto forOp = scf::ForOp::create(
                    builder, loc, (*c0Ptr1), (*FHPtr1), (*c1Ptr1), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues5, 1)),
                    [ivOHPtr1, d0Ptr2, strHeightPtr2, d1Ptr2, dilHeightPtr2, c0Ptr2, FWPtr2, c1Ptr2, ivOWPtr1, strWidthPtr2, dilWidthPtr2, ivNPtr1, ivOCPtr, vecTyPtr2, inputPtr2, filterPtr2, elemTyPtr2](OpBuilder &builder, Location loc, Value ivFH,
                        ValueRange iargs) {
                      mlir::Value listValues6[] = {(*ivOHPtr1), ivFH};
                      Value rowInput = affine::AffineApplyOp::create(
                          builder, loc,
                          AffineMap::get(2, 0, (*d0Ptr2) * (*strHeightPtr2) + (*d1Ptr2) * (*dilHeightPtr2)),
                          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues6, 2)));
                      Value rowFilter = ivFH;
                      // FW
                      mlir::Value listValues7[] = {iargs[0]};
                      auto *ivOWPtr2 = ivOWPtr1;
                      auto *d0Ptr3 = d0Ptr2;
                      auto *strWidthPtr3 = strWidthPtr2;
                      auto *d1Ptr3 = d1Ptr2;
                      auto *dilWidthPtr3 = dilWidthPtr2;
                      auto *ivNPtr2 = ivNPtr1;
                      auto *rowInputPtr = &rowInput;
                      auto *ivOCPtr1 = ivOCPtr;
                      auto *vecTyPtr3 = vecTyPtr2;
                      auto *inputPtr3 = inputPtr2;
                      auto *rowFilterPtr = &rowFilter;
                      auto *filterPtr3 = filterPtr2;
                      auto *elemTyPtr3 = elemTyPtr2;
                      auto forOp = scf::ForOp::create(
                          builder, loc, (*c0Ptr2), (*FWPtr2), (*c1Ptr2), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues7, 1)),
                          [ivOWPtr2, d0Ptr3, strWidthPtr3, d1Ptr3, dilWidthPtr3, ivNPtr2, rowInputPtr, ivOCPtr1, vecTyPtr3, inputPtr3, rowFilterPtr, filterPtr3, elemTyPtr3](OpBuilder &builder, Location loc, Value ivFW,
                              ValueRange iargs) {
                            mlir::Value listValues8[] = {(*ivOWPtr2), ivFW};
                            Value columnInput = affine::AffineApplyOp::create(
                                builder, loc,
                                AffineMap::get(2, 0,
                                               (*d0Ptr3) * (*strWidthPtr3) + (*d1Ptr3) * (*dilWidthPtr3)),
                                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues8, 2)));
                            Value columnFilter = affine::AffineApplyOp::create(
                                builder, loc, AffineMap::get(1, 0, (*d0Ptr3)), ivFW);
                            mlir::Value listValues9[] = {(*ivNPtr2), (*rowInputPtr), columnInput, (*ivOCPtr1)};
                            Value iVec = vector::LoadOp::create(
                                builder, loc, (*vecTyPtr3), (*inputPtr3),
                                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues9, 4)));
                            mlir::Value listValues10[] = {(*rowFilterPtr), columnFilter, (*ivOCPtr1)};
                            Value fVec = vector::LoadOp::create(
                                builder, loc, (*vecTyPtr3), (*filterPtr3),
                                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues10, 3)));
                            Value tVecNext;
                            if (isa<IntegerType>((*elemTyPtr3))) {
                              Value mulVec = arith::MulIOp::create(builder, loc,
                                                                   iVec, fVec);
                              tVecNext = arith::AddIOp::create(
                                  builder, loc, mulVec, iargs[0]);
                            } else {
                              tVecNext = vector::FMAOp::create(
                                  builder, loc, (*vecTyPtr3), iVec, fVec, iargs[0]);
                            }

                            mlir::Value listValues11[] = {tVecNext};
                            scf::YieldOp::create(builder, loc,
                                                 mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues11, 1)));
                          });
                      mlir::Value listValues12[] = {forOp.getResult(0)};
                      scf::YieldOp::create(builder, loc,
                                           mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues12, 1)));
                    });
                mlir::Value listValues13[] = {(*ivNPtr), (*ivOHPtr), (*ivOWPtr), ivOC};
                vector::StoreOp::create(builder, loc, forOp.getResult(0),
                                        (*outputPtr1),
                                        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues13, 4)));

                scf::YieldOp::create(builder, loc, ValueRange{});
              });

          // applyOC
          Value condition = arith::CmpIOp::create(
              nestedBuilder, (*locPtr), arith::CmpIPredicate::sgt, (*tailLengthPtr), (*c0Ptr));
          auto *ivNPtr3 = &ivN;
          auto *ivOHPtr2 = &ivOH;
          auto *ivOWPtr3 = &ivOW;
          auto *applyOCPtr1 = applyOCPtr;
          auto *vecTyPtr4 = vecTyPtr;
          auto *outputPtr2 = outputPtr;
          auto *maskVectorPtr1 = maskVectorPtr;
          auto *zeroElementTypeVecPtr1 = zeroElementTypeVecPtr;
          auto *c0Ptr3 = c0Ptr;
          auto *FHPtr2 = FHPtr;
          auto *c1Ptr3 = c1Ptr;
          auto *d0Ptr4 = d0Ptr;
          auto *strHeightPtr3 = strHeightPtr;
          auto *d1Ptr4 = d1Ptr;
          auto *dilHeightPtr3 = dilHeightPtr;
          auto *FWPtr3 = FWPtr;
          auto *strWidthPtr4 = strWidthPtr;
          auto *dilWidthPtr4 = dilWidthPtr;
          auto *inputPtr4 = inputPtr;
          auto *filterPtr4 = filterPtr;
          auto *elemTyPtr4 = elemTyPtr;
          scf::IfOp::create(
              nestedBuilder, (*locPtr), condition,
              [ivNPtr3, ivOHPtr2, ivOWPtr3, applyOCPtr1, vecTyPtr4, outputPtr2, maskVectorPtr1, zeroElementTypeVecPtr1, c0Ptr3, FHPtr2, c1Ptr3, d0Ptr4, strHeightPtr3, d1Ptr4, dilHeightPtr3, FWPtr3, strWidthPtr4, dilWidthPtr4, inputPtr4, filterPtr4, elemTyPtr4](OpBuilder &builder, Location loc) {
                mlir::Value listValues14[] = {(*ivNPtr3), (*ivOHPtr2), (*ivOWPtr3), (*applyOCPtr1)};
                Value tVec = vector::MaskedLoadOp::create(
                    builder, loc, (*vecTyPtr4), (*outputPtr2),
                    mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues14, 4)), (*maskVectorPtr1),
                    (*zeroElementTypeVecPtr1));
                // FH
                mlir::Value listValues15[] = {tVec};
                auto *ivOHPtr3 = ivOHPtr2;
                auto *d0Ptr5 = d0Ptr4;
                auto *strHeightPtr4 = strHeightPtr3;
                auto *d1Ptr5 = d1Ptr4;
                auto *dilHeightPtr4 = dilHeightPtr3;
                auto *c0Ptr4 = c0Ptr3;
                auto *FWPtr4 = FWPtr3;
                auto *c1Ptr4 = c1Ptr3;
                auto *ivOWPtr4 = ivOWPtr3;
                auto *strWidthPtr5 = strWidthPtr4;
                auto *dilWidthPtr5 = dilWidthPtr4;
                auto *ivNPtr4 = ivNPtr3;
                auto *applyOCPtr2 = applyOCPtr1;
                auto *vecTyPtr5 = vecTyPtr4;
                auto *inputPtr5 = inputPtr4;
                auto *maskVectorPtr2 = maskVectorPtr1;
                auto *zeroElementTypeVecPtr2 = zeroElementTypeVecPtr1;
                auto *filterPtr5 = filterPtr4;
                auto *elemTyPtr5 = elemTyPtr4;
                auto forOp = scf::ForOp::create(
                    builder, loc, (*c0Ptr3), (*FHPtr2), (*c1Ptr3), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues15, 1)),
                    [ivOHPtr3, d0Ptr5, strHeightPtr4, d1Ptr5, dilHeightPtr4, c0Ptr4, FWPtr4, c1Ptr4, ivOWPtr4, strWidthPtr5, dilWidthPtr5, ivNPtr4, applyOCPtr2, vecTyPtr5, inputPtr5, maskVectorPtr2, zeroElementTypeVecPtr2, filterPtr5, elemTyPtr5](OpBuilder &builder, Location loc, Value ivFH,
                        ValueRange iargs) {
                      mlir::Value listValues16[] = {(*ivOHPtr3), ivFH};
                      Value rowInput = affine::AffineApplyOp::create(
                          builder, loc,
                          AffineMap::get(2, 0, (*d0Ptr5) * (*strHeightPtr4) + (*d1Ptr5) * (*dilHeightPtr4)),
                          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues16, 2)));
                      Value rowFilter = ivFH;
                      // FW
                      mlir::Value listValues17[] = {iargs[0]};
                      auto *ivOWPtr5 = ivOWPtr4;
                      auto *d0Ptr6 = d0Ptr5;
                      auto *strWidthPtr6 = strWidthPtr5;
                      auto *d1Ptr6 = d1Ptr5;
                      auto *dilWidthPtr6 = dilWidthPtr5;
                      auto *ivNPtr5 = ivNPtr4;
                      auto *rowInputPtr1 = &rowInput;
                      auto *applyOCPtr3 = applyOCPtr2;
                      auto *vecTyPtr6 = vecTyPtr5;
                      auto *inputPtr6 = inputPtr5;
                      auto *maskVectorPtr3 = maskVectorPtr2;
                      auto *zeroElementTypeVecPtr3 = zeroElementTypeVecPtr2;
                      auto *rowFilterPtr1 = &rowFilter;
                      auto *filterPtr6 = filterPtr5;
                      auto *elemTyPtr6 = elemTyPtr5;
                      auto forOp = scf::ForOp::create(
                          builder, loc, (*c0Ptr4), (*FWPtr4), (*c1Ptr4), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues17, 1)),
                          [ivOWPtr5, d0Ptr6, strWidthPtr6, d1Ptr6, dilWidthPtr6, ivNPtr5, rowInputPtr1, applyOCPtr3, vecTyPtr6, inputPtr6, maskVectorPtr3, zeroElementTypeVecPtr3, rowFilterPtr1, filterPtr6, elemTyPtr6](OpBuilder &builder, Location loc, Value ivFW,
                              ValueRange iargs) {
                            mlir::Value listValues18[] = {(*ivOWPtr5), ivFW};
                            Value columnInput = affine::AffineApplyOp::create(
                                builder, loc,
                                AffineMap::get(2, 0,
                                               (*d0Ptr6) * (*strWidthPtr6) + (*d1Ptr6) * (*dilWidthPtr6)),
                                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues18, 2)));
                            Value columnFilter = affine::AffineApplyOp::create(
                                builder, loc, AffineMap::get(1, 0, (*d0Ptr6)), ivFW);
                            mlir::Value listValues19[] = {(*ivNPtr5), (*rowInputPtr1), columnInput, (*applyOCPtr3)};
                            Value iVec = vector::MaskedLoadOp::create(
                                builder, loc, (*vecTyPtr6), (*inputPtr6),
                                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues19, 4)),
                                (*maskVectorPtr3), (*zeroElementTypeVecPtr3));
                            mlir::Value listValues20[] = {(*rowFilterPtr1), columnFilter, (*applyOCPtr3)};
                            Value fVec = vector::MaskedLoadOp::create(
                                builder, loc, (*vecTyPtr6), (*filterPtr6),
                                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues20, 3)),
                                (*maskVectorPtr3), (*zeroElementTypeVecPtr3));
                            Value tVecNext;
                            if (isa<IntegerType>((*elemTyPtr6))) {
                              Value mulVec = arith::MulIOp::create(builder, loc,
                                                                   iVec, fVec);
                              tVecNext = arith::AddIOp::create(
                                  builder, loc, mulVec, iargs[0]);
                            } else {
                              tVecNext = vector::FMAOp::create(
                                  builder, loc, (*vecTyPtr6), iVec, fVec, iargs[0]);
                            }

                            mlir::Value listValues21[] = {tVecNext};
                            scf::YieldOp::create(builder, loc,
                                                 mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues21, 1)));
                          });
                      mlir::Value listValues22[] = {forOp.getResult(0)};
                      scf::YieldOp::create(builder, loc,
                                           mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues22, 1)));
                    });
                mlir::Value listValues23[] = {(*ivNPtr3), (*ivOHPtr2), (*ivOWPtr3), (*applyOCPtr1)};
                vector::MaskedStoreOp::create(
                    builder, loc, (*outputPtr2), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues23, 4)),
                    (*maskVectorPtr1), forOp.getResult(0));
                scf::YieldOp::create(builder, loc, ValueRange{});
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
} // end anonymous namespace

//===----------------------------------------------------------------------===//
// DepthwiseConv2DNhwcHwcOptimizePass
//===----------------------------------------------------------------------===//

namespace DepthwiseConvNhwcHwc_anonymousNamespace_01 {
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
} // end anonymous namespace.

void DepthwiseConvNhwcHwc_anonymousNamespace_01::DepthwiseConv2DNhwcHwcOptimizePass::runOnOperation() {
  MLIRContext *context = &getContext();
  ModuleOp module = getOperation();

  ConversionTarget target(*context);
  target
      .addLegalDialect<arith::ArithDialect, affine::AffineDialect,
                       scf::SCFDialect, memref::MemRefDialect, VectorDialect>();
  target.addLegalOp<ModuleOp, func::FuncOp, func::ReturnOp>();
  target.addLegalOp<linalg::FillOp>();

  RewritePatternSet patterns(context);
  patterns.add<DepthwiseConvNhwcHwc_anonymousNamespace_01::DepthwiseConv2DNhwcHwcOptimizePattern>(context, vecSize);

  if (failed(applyPartialConversion(module, target, std::move(patterns))))
    signalPassFailure();
}

namespace mlir {
namespace buddy {
void registerDepthwiseConv2DNhwcHwcOptimizePass() {
  PassRegistration<DepthwiseConvNhwcHwc_anonymousNamespace_01::DepthwiseConv2DNhwcHwcOptimizePass>();
}
} // namespace buddy
} // namespace mlir
