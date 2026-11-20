#ifndef FC_MLIR_DIALECT_LINALG_IR_LINALGOPS_H
#define FC_MLIR_DIALECT_LINALG_IR_LINALGOPS_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Operation.h"

namespace mlir {
namespace linalg {

class FillOp : public Op<FillOp> {
public:
  FillOp() : Op<FillOp>(0) {}
   explicit FillOp(Operation *op) : Op<FillOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);
};

/* 官方 linalg.depthwise_conv_2d_nhwc_hwc：NHWC 输入/输出 + HWC 卷积核。 */
class DepthwiseConv2DNhwcHwcOp : public Op<DepthwiseConv2DNhwcHwcOp> {
public:
  DepthwiseConv2DNhwcHwcOp() : Op<DepthwiseConv2DNhwcHwcOp>(0) {}
  explicit DepthwiseConv2DNhwcHwcOp(Operation *op)
      : Op<DepthwiseConv2DNhwcHwcOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  /* 官方访问器：strides/dilations 返回 DenseIntElementsAttr（另有 *Attr() 版本）。 */
  DenseIntElementsAttr getStrides();
  DenseIntElementsAttr getDilations();
};

} // namespace linalg
} // namespace mlir
#endif
