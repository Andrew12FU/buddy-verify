// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded declarations; see docs/PROVENANCE.md for retained and omitted
// parts.
#ifndef BUDDY_MINI_MLIR_IR_LOCATION_H
#define BUDDY_MINI_MLIR_IR_LOCATION_H
#include "mlir/IR/Attributes.h"

namespace mlir {
class LocationAttr : public Attribute {
public:
  using Attribute::Attribute;
};
class Location {
public:
  Location(LocationAttr loc);
  Location(const Location &) = default;

private:
  LocationAttr impl;
};
} // namespace mlir
#endif
