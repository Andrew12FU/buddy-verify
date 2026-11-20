// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "model/Registry.h"
namespace buddy::extract {
/// Emit independent skeleton.h and JSON records into a caller-owned directory.
/// @return false on output I/O failure. Unsupported declarations remain
/// explicit in records.json; successful emission is not a compilation or
/// semantic proof.
bool emit(Registry &r, llvm::StringRef directory);
} // namespace buddy::extract
