//===- BuiltinTransposeVectorization.cpp ----------------------------------===//
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
// This file implements the transpose optimization.
//
//===----------------------------------------------------------------------===//
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/IntegerSet.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/ArrayRef.h"
#include <cstdint>
#include <mlir/Dialect/Affine/Analysis/AffineAnalysis.h>
#include <mlir/Dialect/Affine/IR/AffineOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/Linalg/Transforms/Transforms.h>
#include <mlir/IR/Dialect.h>
#include <mlir/IR/Operation.h>
#include <mlir/IR/TypeUtilities.h>
#include <mlir/IR/Value.h>
#include <mlir/Pass/Pass.h>

using namespace mlir;
using namespace vector;
using namespace affine;

//===----------------------------------------------------------------------===//
// Rewrite Pattern
//===----------------------------------------------------------------------===//

namespace BuiltinTransposeVectorization_anonymousNamespace_01 {} using namespace BuiltinTransposeVectorization_anonymousNamespace_01;
namespace BuiltinTransposeVectorization_anonymousNamespace_01 {

class TransposeOptimizationPattern : public ConversionPattern {
public:
  explicit TransposeOptimizationPattern(MLIRContext *context,
                                        int64_t affineVectorSizeParam)
      : ConversionPattern(linalg::TransposeOp::getOperationName(), 1, context) {
    affineVectorSize = affineVectorSizeParam;
  }

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> /*operands*/,
                  ConversionPatternRewriter &rewriter) const override {
    auto permutationArrayAttr =
        mlir::cast<DenseI64ArrayAttr>(
            op->getAttr(rewriter.getStringAttr("permutation")))
            .asArrayRef();
    // Retrieve input tensors A, B.
    Value A = op->getOperand(0);
    Value B = op->getOperand(1);

    // Only to rewrite the rank 2 tensor transpose.
    if (permutationArrayAttr[0] != 1 or permutationArrayAttr[1] != 0 or
        mlir::cast<mlir::MemRefType>(A.getType()).getRank() != 2) {
      return failure();
    }

    auto loc = op->getLoc();

    // Acquire the element type of input tensors.
    Type elementType =
        mlir::cast<mlir::MemRefType>(A.getType()).getElementType();

    // Define constants.
    mlir::Value index0Storage = arith::ConstantOp::create(rewriter, loc, rewriter.getIndexAttr(0));
    const mlir::Value &index0 = index0Storage;
    mlir::Value indexVecSizeStorage = arith::ConstantOp::create(
        rewriter, loc, rewriter.getIndexAttr(affineVectorSize));
    const mlir::Value &indexVecSize = indexVecSizeStorage;
    mlir::AffineExpr d0Storage = rewriter.getAffineDimExpr(0);
    const mlir::AffineExpr &d0 = d0Storage;
    mlir::AffineExpr d1Storage = rewriter.getAffineDimExpr(1);
    const mlir::AffineExpr &d1 = d1Storage;
    mlir::AffineExpr s0Storage = rewriter.getAffineSymbolExpr(0);
    const mlir::AffineExpr &s0 = s0Storage;
    mlir::AffineExpr zeroAffineStorage = rewriter.getAffineConstantExpr(0);
    const mlir::AffineExpr &zeroAffine = zeroAffineStorage;

    mlir::Value zeroElementTypeStorage = arith::ConstantOp::create(
        rewriter, loc, rewriter.getZeroAttr(elementType));
    const mlir::Value &zeroElementType = zeroElementTypeStorage;

    // Get dimensions of input tensor.
    Value Row = memref::DimOp::create(rewriter, loc, A, 0);
    Value Col = memref::DimOp::create(rewriter, loc, A, 1);

    // Calculate the length of the tail, which might not fit in a vector.
    mlir::Value listValues[] = {Row};
    Value rowUnalignedLength = affine::AffineApplyOp::create(
        rewriter, loc, AffineMap::get(1, 0, d0 % affineVectorSize),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues, 1)));
    mlir::Value listValues1[] = {Col};
    Value colUnalignedLength = affine::AffineApplyOp::create(
        rewriter, loc, AffineMap::get(1, 0, d0 % affineVectorSize),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues1, 1)));
    mlir::Value listValues2[] = {Row};
    Value rowUpperBound = affine::AffineApplyOp::create(
        rewriter, loc,
        AffineMap::get(1, 0, d0.floorDiv(affineVectorSize) * affineVectorSize),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues2, 1)));

    // Generate a mask vector based on the tail length.
    long listValues3[] = {affineVectorSize, affineVectorSize};
    mlir::Value listValues4[] = {rowUnalignedLength, indexVecSize};
    Value rowEndMaskLoad = vector::CreateMaskOp::create(
        rewriter, loc,
        VectorType::get(llvm::ArrayRef<long>(listValues3, 2),
                        rewriter.getI1Type()),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues4, 2)));
    long listValues5[] = {affineVectorSize, affineVectorSize};
    mlir::Value listValues6[] = {indexVecSize, colUnalignedLength};
    Value colEndMaskLoad = vector::CreateMaskOp::create(
        rewriter, loc,
        VectorType::get(llvm::ArrayRef<long>(listValues5, 2),
                        rewriter.getI1Type()),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues6, 2)));
    long listValues7[] = {affineVectorSize, affineVectorSize};
    mlir::Value listValues8[] = {indexVecSize, rowUnalignedLength};
    Value rowEndMaskStore = vector::CreateMaskOp::create(
        rewriter, loc,
        VectorType::get(llvm::ArrayRef<long>(listValues7, 2),
                        rewriter.getI1Type()),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues8, 2)));
    long listValues9[] = {affineVectorSize, affineVectorSize};
    mlir::Value listValues10[] = {colUnalignedLength, indexVecSize};
    Value colEndMaskStore = vector::CreateMaskOp::create(
        rewriter, loc,
        VectorType::get(llvm::ArrayRef<long>(listValues9, 2),
                        rewriter.getI1Type()),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues10, 2)));

    SmallVector<Value, 4U> reducedValues = llvm::to_vector<4>(
        llvm::map_range(ArrayRef<LoopReduction>{},
                        [](const LoopReduction &red) { return red.value; }));

    // Create the primary parallel loop.
    mlir::Value listValues11[] = {Col};
    int listValues12[] = {1};
    int listValues13[] = {1};
    mlir::AffineExpr listValues14[] = {zeroAffine};
    mlir::AffineExpr listValues15[] = {s0.floorDiv(affineVectorSize) * affineVectorSize};
    long listValues16[] = {affineVectorSize};
    mlir::NamedAttribute listValues71[] = {rewriter.getNamedAttr("lowerBoundsGroups",
                                  rewriter.getI32TensorAttr(llvm::ArrayRef<int>(listValues12, 1))), rewriter.getNamedAttr("upperBoundsGroups",
                                  rewriter.getI32TensorAttr(llvm::ArrayRef<int>(listValues13, 1))), rewriter.getNamedAttr(
                "lowerBoundsMap",
                AffineMapAttr::get(
                    AffineMap::get(0, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues14, 1), rewriter.getContext()))), rewriter.getNamedAttr(
                "upperBoundsMap",
                AffineMapAttr::get(AffineMap::get(
                    0, 1, llvm::ArrayRef<mlir::AffineExpr>(listValues15, 1),
                    rewriter.getContext()))), rewriter.getNamedAttr("reductions", rewriter.getArrayAttr({})), rewriter.getNamedAttr(
                "steps", rewriter.getI64ArrayAttr(llvm::ArrayRef<long>(listValues16, 1)))};
    AffineParallelOp parallelColLoop = affine::AffineParallelOp::create(
        rewriter, loc, ValueRange(reducedValues).getTypes(), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues11, 1)),
        llvm::ArrayRef<mlir::NamedAttribute>(listValues71, 6));

    // Create the loop body for the parallel loop.
    Block *loopBody = new Block();
    rewriter.setInsertionPointToStart(loopBody);
    loopBody->addArgument(rewriter.getIndexType(), loc);
    Value colIdx = loopBody->getArguments()[0];

    mlir::Value listValues17[] = {index0};
    mlir::Value listValues18[] = {rowUpperBound};
    auto *APtr = &A;
    auto *colIdxPtr = &colIdx;
    auto *zeroElementTypePtr = &zeroElementType;
    auto *d0Ptr = &d0;
    auto *d1Ptr = &d1;
    auto *elementTypePtr = &elementType;
    auto *rewriterPtr = &rewriter;
    auto *BPtr = &B;
    auto *RowPtr = &Row;
    auto *rowUpperBoundPtr = &rowUpperBound;
    auto *rowEndMaskLoadPtr = &rowEndMaskLoad;
    auto *rowEndMaskStorePtr = &rowEndMaskStore;
    auto *affineVectorSizePtr = &(this->affineVectorSize);
    affine::buildAffineLoopNest(
        rewriter, loc, mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues17, 1)), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues18, 1)), affineVectorSize,
        [APtr, colIdxPtr, zeroElementTypePtr, d0Ptr, d1Ptr, elementTypePtr, rewriterPtr, BPtr, RowPtr, rowUpperBoundPtr, rowEndMaskLoadPtr, rowEndMaskStorePtr, affineVectorSizePtr](OpBuilder &builder, Location loc, ValueRange ivRange) {
          Value rowIdx = ivRange.front();

          long listValues19[] = {(*affineVectorSizePtr), (*affineVectorSizePtr)};
          mlir::Value listValues20[] = {(*APtr), rowIdx, (*colIdxPtr), (*zeroElementTypePtr)};
          bool listValues21[] = {false, true};
          int listValues22[] = {1, 2, 1, 0};
          mlir::AffineExpr listValues23[] = {(*d0Ptr), (*d1Ptr)};
          mlir::Type listValues72[] = {VectorType::get(
                  llvm::ArrayRef<long>(listValues19, 2),
                  (*elementTypePtr))};
          mlir::NamedAttribute listValues73[] = {(*rewriterPtr).getNamedAttr(
                      "in_bounds",
                      (*rewriterPtr).getBoolArrayAttr(llvm::ArrayRef<bool>(listValues21, 2))), (*rewriterPtr).getNamedAttr(
                      "operand_segment_sizes",
                      (*rewriterPtr).getDenseI32ArrayAttr(llvm::ArrayRef<int>(listValues22, 4))), (*rewriterPtr).getNamedAttr(
                      "permutation_map",
                      AffineMapAttr::get(AffineMap::get(
                          2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues23, 2), (*rewriterPtr).getContext())))};
          auto tiledMatrix = vector::TransferReadOp::create(
              (*rewriterPtr), loc,
              mlir::TypeRange(llvm::ArrayRef<mlir::Type>(listValues72, 1)),
              mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues20, 4)),
              llvm::ArrayRef<mlir::NamedAttribute>(listValues73, 3));

          mlir::Value listValues24[] = {tiledMatrix, (*BPtr), (*colIdxPtr), rowIdx};
          bool listValues25[] = {true, true};
          int listValues26[] = {1, 1, 2, 0};
          mlir::AffineExpr listValues27[] = {(*d1Ptr), (*d0Ptr)};
          mlir::NamedAttribute listValues74[] = {(*rewriterPtr).getNamedAttr(
                      "in_bounds",
                      (*rewriterPtr).getBoolArrayAttr(llvm::ArrayRef<bool>(listValues25, 2))), (*rewriterPtr).getNamedAttr(
                      "operand_segment_sizes",
                      (*rewriterPtr).getDenseI32ArrayAttr(llvm::ArrayRef<int>(listValues26, 4))), (*rewriterPtr).getNamedAttr(
                      "permutation_map",
                      AffineMapAttr::get(AffineMap::get(2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues27, 2),
                                                        builder.getContext())))};
          vector::TransferWriteOp::create(
              (*rewriterPtr), loc, TypeRange{},
              mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues24, 4)),
              llvm::ArrayRef<mlir::NamedAttribute>(listValues74, 3));

          // Compile time branch detection.
          if (mlir::cast<mlir::MemRefType>((*APtr).getType()).isDynamicDim(0) or
              mlir::cast<mlir::MemRefType>((*APtr).getType()).getDimSize(0) %
                      (*affineVectorSizePtr) !=
                  0) {
            // Depending on the position, use either full vectors or tail
            // vectors.
            mlir::AffineExpr listValues28[] = {(*d0Ptr) % (*affineVectorSizePtr) - 1};
            bool listValues29[] = {false};
            mlir::Value listValues30[] = {(*RowPtr)};
            affine::AffineIfOp branchingRowUnaligned =
                affine::AffineIfOp::create(
                    builder, loc,
                    IntegerSet::get(1, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues28, 1), llvm::ArrayRef<bool>(listValues29, 1)),
                    mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues30, 1)), false);

            // Branch handling unaligned rows.
            OpBuilder trueRowUnalignedBranchBuilder =
                branchingRowUnaligned.getThenBodyBuilder();

            long listValues31[] = {(*affineVectorSizePtr), (*affineVectorSizePtr)};
            mlir::Value listValues32[] = {(*APtr), (*rowUpperBoundPtr), (*colIdxPtr), (*zeroElementTypePtr), (*rowEndMaskLoadPtr)};
            bool listValues33[] = {false, true};
            int listValues34[] = {1, 2, 1, 1};
            mlir::AffineExpr listValues35[] = {(*d0Ptr), (*d1Ptr)};
            mlir::Type listValues75[] = {VectorType::get(
                    llvm::ArrayRef<long>(listValues31, 2),
                    (*elementTypePtr))};
            mlir::NamedAttribute listValues76[] = {(*rewriterPtr).getNamedAttr(
                        "in_bounds",
                        (*rewriterPtr).getBoolArrayAttr(llvm::ArrayRef<bool>(listValues33, 2))), (*rewriterPtr).getNamedAttr("operand_segment_sizes",
                                          (*rewriterPtr).getDenseI32ArrayAttr(
                                              llvm::ArrayRef<int>(listValues34, 4))), (*rewriterPtr).getNamedAttr(
                        "permutation_map",
                        AffineMapAttr::get(AffineMap::get(
                            2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues35, 2), (*rewriterPtr).getContext())))};
            auto rowUnalignedTiledMatrix = vector::TransferReadOp::create(
                trueRowUnalignedBranchBuilder, loc,
                mlir::TypeRange(llvm::ArrayRef<mlir::Type>(listValues75, 1)),
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues32, 5)),
                llvm::ArrayRef<mlir::NamedAttribute>(listValues76, 3));
            mlir::Value listValues36[] = {rowUnalignedTiledMatrix, (*BPtr), (*colIdxPtr), (*rowUpperBoundPtr), (*rowEndMaskStorePtr)};
            bool listValues37[] = {true, false};
            int listValues38[] = {1, 1, 2, 1};
            mlir::AffineExpr listValues39[] = {(*d1Ptr), (*d0Ptr)};
            mlir::NamedAttribute listValues77[] = {(*rewriterPtr).getNamedAttr(
                        "in_bounds",
                        (*rewriterPtr).getBoolArrayAttr(llvm::ArrayRef<bool>(listValues37, 2))), (*rewriterPtr).getNamedAttr("operand_segment_sizes",
                                          (*rewriterPtr).getDenseI32ArrayAttr(
                                              llvm::ArrayRef<int>(listValues38, 4))), (*rewriterPtr).getNamedAttr(
                        "permutation_map",
                        AffineMapAttr::get(AffineMap::get(
                            2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues39, 2), builder.getContext())))};
            vector::TransferWriteOp::create(
                trueRowUnalignedBranchBuilder, loc, TypeRange{},
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues36, 5)),
                llvm::ArrayRef<mlir::NamedAttribute>(listValues77, 3));
          }
        });

    affine::AffineYieldOp::create(rewriter, loc);

    // Finalize the loop.
    parallelColLoop.getRegion().push_back(loopBody);
    rewriter.setInsertionPointAfter(parallelColLoop);

    if (mlir::cast<mlir::MemRefType>(A.getType()).isDynamicDim(1) or
        mlir::cast<mlir::MemRefType>(A.getType()).getDimSize(1) %
                affineVectorSize !=
            0) {

      mlir::AffineExpr listValues40[] = {d0 % affineVectorSize - 1};
      bool listValues41[] = {false};
      mlir::Value listValues42[] = {Col};
      affine::AffineIfOp branchingColUnaligned = affine::AffineIfOp::create(
          rewriter, loc,
          IntegerSet::get(1, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues40, 1), llvm::ArrayRef<bool>(listValues41, 1)),
          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues42, 1)), false);

      OpBuilder trueColUnalignedBranchBuilder =
          branchingColUnaligned.getThenBodyBuilder();
      mlir::Value listValues43[] = {Col};
      Value colUpperBound = affine::AffineApplyOp::create(
          trueColUnalignedBranchBuilder, loc,
          AffineMap::get(1, 0,
                         d0.floorDiv(affineVectorSize) * affineVectorSize),
          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues43, 1)));

      mlir::Value listValues44[] = {index0};
      mlir::Value listValues45[] = {rowUpperBound};
      auto *APtr1 = &A;
      auto *colUpperBoundPtr = &colUpperBound;
      auto *zeroElementTypePtr1 = &zeroElementType;
      auto *colEndMaskLoadPtr = &colEndMaskLoad;
      auto *d0Ptr1 = &d0;
      auto *d1Ptr1 = &d1;
      auto *elementTypePtr1 = &elementType;
      auto *BPtr1 = &B;
      auto *colEndMaskStorePtr = &colEndMaskStore;
      auto *rewriterPtr1 = &rewriter;
      auto *affineVectorSizePtr1 = &(this->affineVectorSize);
      affine::buildAffineLoopNest(
          trueColUnalignedBranchBuilder, loc, mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues44, 1)), mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues45, 1)),
          affineVectorSize,
          [APtr1, colUpperBoundPtr, zeroElementTypePtr1, colEndMaskLoadPtr, d0Ptr1, d1Ptr1, elementTypePtr1, BPtr1, colEndMaskStorePtr, rewriterPtr1, affineVectorSizePtr1](OpBuilder &builder, Location loc, ValueRange ivRange) {
            Value rowIdx = ivRange.front();
            long listValues46[] = {(*affineVectorSizePtr1), (*affineVectorSizePtr1)};
            mlir::Value listValues47[] = {(*APtr1), rowIdx, (*colUpperBoundPtr), (*zeroElementTypePtr1), (*colEndMaskLoadPtr)};
            bool listValues48[] = {false, false};
            int listValues49[] = {1, 2, 1, 1};
            mlir::AffineExpr listValues50[] = {(*d0Ptr1), (*d1Ptr1)};
            mlir::Type listValues78[] = {VectorType::get(
                    llvm::ArrayRef<long>(listValues46, 2),
                    (*elementTypePtr1))};
            mlir::NamedAttribute listValues79[] = {builder.getNamedAttr(
                        "in_bounds",
                        builder.getBoolArrayAttr(llvm::ArrayRef<bool>(listValues48, 2))), builder.getNamedAttr("operand_segment_sizes",
                                         builder.getDenseI32ArrayAttr(
                                             llvm::ArrayRef<int>(listValues49, 4))), builder.getNamedAttr(
                        "permutation_map",
                        AffineMapAttr::get(AffineMap::get(
                            2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues50, 2), builder.getContext())))};
            auto colUnalignedTiledMatrix = vector::TransferReadOp::create(
                builder, loc,
                mlir::TypeRange(llvm::ArrayRef<mlir::Type>(listValues78, 1)),
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues47, 5)),
                llvm::ArrayRef<mlir::NamedAttribute>(listValues79, 3));
            mlir::Value listValues51[] = {colUnalignedTiledMatrix, (*BPtr1), (*colUpperBoundPtr), rowIdx, (*colEndMaskStorePtr)};
            bool listValues52[] = {false, true};
            int listValues53[] = {1, 1, 2, 1};
            mlir::AffineExpr listValues54[] = {(*d1Ptr1), (*d0Ptr1)};
            mlir::NamedAttribute listValues80[] = {(*rewriterPtr1).getNamedAttr(
                        "in_bounds",
                        (*rewriterPtr1).getBoolArrayAttr(llvm::ArrayRef<bool>(listValues52, 2))), (*rewriterPtr1).getNamedAttr("operand_segment_sizes",
                                          (*rewriterPtr1).getDenseI32ArrayAttr(
                                              llvm::ArrayRef<int>(listValues53, 4))), (*rewriterPtr1).getNamedAttr(
                        "permutation_map",
                        AffineMapAttr::get(AffineMap::get(
                            2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues54, 2), builder.getContext())))};
            vector::TransferWriteOp::create(
                builder, loc, TypeRange{},
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues51, 5)),
                llvm::ArrayRef<mlir::NamedAttribute>(listValues80, 3));
          });

      if (mlir::cast<mlir::MemRefType>(A.getType()).isDynamicDim(0) or
          mlir::cast<mlir::MemRefType>(A.getType()).getDimSize(0) %
                  affineVectorSize !=
              0) {
        mlir::AffineExpr listValues55[] = {d0 % affineVectorSize - 1};
        bool listValues56[] = {false};
        mlir::Value listValues57[] = {Col};
        affine::AffineIfOp branchingRowColUnaligned =
            affine::AffineIfOp::create(
                trueColUnalignedBranchBuilder, loc,
                IntegerSet::get(1, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues55, 1), llvm::ArrayRef<bool>(listValues56, 1)),
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues57, 1)), false);

        OpBuilder trueRowColUnalignedBranchBuilder =
            branchingRowColUnaligned.getThenBodyBuilder();
        long listValues58[] = {affineVectorSize, affineVectorSize};
        mlir::Value listValues59[] = {rowUnalignedLength, colUnalignedLength};
        Value rowColEndMaskLoad = vector::CreateMaskOp::create(
            trueRowColUnalignedBranchBuilder, loc,
            VectorType::get(llvm::ArrayRef<long>(listValues58, 2),
                            trueRowColUnalignedBranchBuilder.getI1Type()),
            mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues59, 2)));
        long listValues60[] = {affineVectorSize, affineVectorSize};
        mlir::Value listValues61[] = {colUnalignedLength, rowUnalignedLength};
        Value rowColEndMaskStore = vector::CreateMaskOp::create(
            trueRowColUnalignedBranchBuilder, loc,
            VectorType::get(llvm::ArrayRef<long>(listValues60, 2),
                            trueRowColUnalignedBranchBuilder.getI1Type()),
            mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues61, 2)));
        long listValues62[] = {affineVectorSize, affineVectorSize};
        mlir::Value listValues63[] = {A, rowUpperBound, colUpperBound, zeroElementType, rowColEndMaskLoad};
        bool listValues64[] = {false, false};
        int listValues65[] = {1, 2, 1, 1};
        mlir::AffineExpr listValues66[] = {d0, d1};
        mlir::Type listValues81[] = {VectorType::get(
                llvm::ArrayRef<long>(listValues62, 2),
                elementType)};
        mlir::NamedAttribute listValues82[] = {trueRowColUnalignedBranchBuilder.getNamedAttr(
                    "in_bounds",
                    trueRowColUnalignedBranchBuilder.getBoolArrayAttr(
                        llvm::ArrayRef<bool>(listValues64, 2))), trueRowColUnalignedBranchBuilder.getNamedAttr(
                    "operand_segment_sizes",
                    trueRowColUnalignedBranchBuilder.getDenseI32ArrayAttr(
                        llvm::ArrayRef<int>(listValues65, 4))), trueRowColUnalignedBranchBuilder.getNamedAttr(
                    "permutation_map",
                    AffineMapAttr::get(AffineMap::get(
                        2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues66, 2),
                        trueRowColUnalignedBranchBuilder.getContext())))};
        auto rowColUnalignedTiledMatrix = vector::TransferReadOp::create(
            trueRowColUnalignedBranchBuilder, loc,
            mlir::TypeRange(llvm::ArrayRef<mlir::Type>(listValues81, 1)),
            mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues63, 5)),
            llvm::ArrayRef<mlir::NamedAttribute>(listValues82, 3));
        mlir::Value listValues67[] = {rowColUnalignedTiledMatrix, B, colUpperBound, rowUpperBound, rowColEndMaskStore};
        bool listValues68[] = {false, false};
        int listValues69[] = {1, 1, 2, 1};
        mlir::AffineExpr listValues70[] = {d1, d0};
        mlir::NamedAttribute listValues83[] = {rewriter.getNamedAttr(
                    "in_bounds",
                    rewriter.getBoolArrayAttr(llvm::ArrayRef<bool>(listValues68, 2))), rewriter.getNamedAttr(
                    "operand_segment_sizes",
                    rewriter.getDenseI32ArrayAttr(llvm::ArrayRef<int>(listValues69, 4))), rewriter.getNamedAttr(
                    "permutation_map",
                    AffineMapAttr::get(AffineMap::get(
                        2, 0, llvm::ArrayRef<mlir::AffineExpr>(listValues70, 2),
                        trueRowColUnalignedBranchBuilder.getContext())))};
        vector::TransferWriteOp::create(
            trueRowColUnalignedBranchBuilder, loc, TypeRange{},
            mlir::ValueRange(llvm::ArrayRef<mlir::Value>(listValues67, 5)),
            llvm::ArrayRef<mlir::NamedAttribute>(listValues83, 3));
      }
    }

    // Erase the original operation.
    rewriter.eraseOp(op);
    return success();
  }

private:
  int64_t affineVectorSize;
};
} // end anonymous namespace

//===----------------------------------------------------------------------===//
// TransposeOptimizationPass
//===----------------------------------------------------------------------===//

/// This is a partial lowering linalg pooling operations to mixture of
/// Affine + Vector operations.
namespace BuiltinTransposeVectorization_anonymousNamespace_01 {} using namespace BuiltinTransposeVectorization_anonymousNamespace_01;
namespace BuiltinTransposeVectorization_anonymousNamespace_01 {
class TransposeOptimizationPass
    : public PassWrapper<TransposeOptimizationPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TransposeOptimizationPass)
  StringRef getArgument() const final { return "transpose-optimize"; }
  StringRef getDescription() const final {
    return "Transpose Optimization only for rank 2 tensor.";
  }
  TransposeOptimizationPass() = default;
  TransposeOptimizationPass(const TransposeOptimizationPass &) {}
  explicit TransposeOptimizationPass(int64_t affineVectorSizeParam) {
    affineVectorSize = affineVectorSizeParam;
  }

  void runOnOperation() override;

  void getDependentDialects(DialectRegistry &registry) const override {
    registry
        .insert<linalg::LinalgDialect, affine::AffineDialect, VectorDialect>();
  }

  Option<int64_t> affineVectorSize{*this, "vector-size",
                                   llvm::cl::desc("Affine Vector size."),
                                   llvm::cl::init(16)};
};
} // end anonymous namespace.

void BuiltinTransposeVectorization_anonymousNamespace_01::TransposeOptimizationPass::runOnOperation() {
  MLIRContext *context = &getContext();
  ModuleOp module = getOperation();

  ConversionTarget target(*context);
  target.addLegalDialect<arith::ArithDialect, affine::AffineDialect,
                         memref::MemRefDialect, VectorDialect>();
  target.addLegalOp<ModuleOp, func::FuncOp, func::ReturnOp>();
  target.addLegalOp<linalg::FillOp>();

  RewritePatternSet patterns(context);
  patterns.add<BuiltinTransposeVectorization_anonymousNamespace_01::TransposeOptimizationPattern>(context, affineVectorSize);

  if (failed(applyPartialConversion(module, target, std::move(patterns))))
    signalPassFailure();
}

namespace mlir {
namespace buddy {
void registerTransposeOptimizationPass() {
  PassRegistration<BuiltinTransposeVectorization_anonymousNamespace_01::TransposeOptimizationPass>();
}
} // namespace buddy
} // namespace mlir
