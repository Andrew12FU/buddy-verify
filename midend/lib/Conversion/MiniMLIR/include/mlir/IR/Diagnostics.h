// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_MLIR_IR_DIAGNOSTICS_H
#define BUDDY_MINI_MLIR_IR_DIAGNOSTICS_H
#include "llvm/ADT/Twine.h"
namespace mlir { class DiagnosticEngine;
class InFlightDiagnostic { DiagnosticEngine *owner=nullptr;
public: InFlightDiagnostic()=default; InFlightDiagnostic(InFlightDiagnostic &&rhs); ~InFlightDiagnostic(); };
}

#endif
