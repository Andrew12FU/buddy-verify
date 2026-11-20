// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "model/Registry.h"
namespace buddy::extract {
/// Visit only main-file AST uses, including lambda bodies and implicit
/// expressions.
/// @param r registry receiving types, selected declarations and operation
/// evidence.
/// @return nothing; never traverses external ordinary method bodies.
void collect(Registry &r);
/// Record the exact cast kind and selected source/target types from an AST
/// cast. Inheritance edges are evidence, not authorization to restore a class
/// hierarchy.
bool receiverOnlyCast(clang::ASTContext &a, const clang::CastExpr *e);
void recordCast(Registry &r, const clang::CastExpr *e);
} // namespace buddy::extract
