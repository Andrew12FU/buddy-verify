#ifndef FC_MLIR_IR_DIALECT_H
#define FC_MLIR_IR_DIALECT_H

#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Types.h"

namespace mlir {

/* 官方 Dialect：构造是 protected 的 (name, context, typeID)，另有虚析构与
 * getNamespace/getContext 等。模型只保留派生类需要的部分。 */
class Dialect {
public:
  virtual ~Dialect();

  MLIRContext *getContext() const;
  StringRef getNamespace() const;

protected:
  Dialect(StringRef name, MLIRContext *context, TypeID typeID);

private:
  void *impl;
};

} // namespace mlir
#endif
