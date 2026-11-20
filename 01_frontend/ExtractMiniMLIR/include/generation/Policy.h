// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "clang/AST/DeclCXX.h"
namespace buddy::extract {
/// Finite specialization policy: type parameters only, no
/// constraints/defaults/packs.
bool supportsSpecialization(const clang::CXXRecordDecl *d);
/// @return the real void walk(Callback[&&/&]) template pattern when the bounded
/// no-callback-execution rule applies, otherwise nullptr. Restricted to MLIR
/// members; every application is recorded against the real declaration USR.
const clang::FunctionDecl *emptyWalkPattern(const clang::FunctionDecl *f);
} // namespace buddy::extract
