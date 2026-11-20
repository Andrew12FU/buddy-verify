#ifndef FC_MLIR_DIALECT_FUNC_IR_FUNCOPS_H
#define FC_MLIR_DIALECT_FUNC_IR_FUNCOPS_H

#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"

namespace mlir {
namespace func {

class FuncOp : public Op<FuncOp> {
public:
  FuncOp() : Op<FuncOp>(0) {}
   explicit FuncOp(Operation *op) : Op<FuncOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);

  bool isDeclaration();
  bool isExternal();
  Region &getBody();
  unsigned getNumArguments();
  BlockArgument getArgument(unsigned index);
  FunctionType getFunctionType();
  void setType(Type type);
  llvm::ArrayRef<Type> getArgumentTypes();
};

class ReturnOp : public Op<ReturnOp> {
public:
  ReturnOp() : Op<ReturnOp>(0) {}
   explicit ReturnOp(Operation *op) : Op<ReturnOp>(op) {}

  static StringRef getOperationName();
  static bool classof(Operation *op);
};

} // namespace func
} // namespace mlir
#endif
