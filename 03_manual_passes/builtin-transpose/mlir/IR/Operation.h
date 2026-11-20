#ifndef MLIR_IR_OPERATION_H
#define MLIR_IR_OPERATION_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Value.h"

namespace mlir {

class Operation {
public:
  Attribute getAttr(StringAttr) const { return Attribute(); }
  Value getOperand(unsigned) const { return Value(); }
  Location getLoc() const { return Location(); }
};

class Block {
public:
  void addArgument(Type, Location) {}
  ArrayRef<Value> getArguments() const { return ArrayRef<Value>(); }
};

class Region {
public:
  void push_back(Block *) {}
};

} // namespace mlir

#endif
