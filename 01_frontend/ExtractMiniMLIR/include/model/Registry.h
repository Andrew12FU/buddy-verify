// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/JSON.h"
#include <map>
#include <set>
namespace buddy::extract {
/// Generation states are monotonic: reference-only types can later acquire
/// members.
enum class State { Forward, Complete };
struct TypeRecord {
  const clang::TagDecl *decl = nullptr;
  State state = State::Forward;
  std::set<std::string> reasons;
  std::string generated = "pending";
  std::map<std::string, const clang::NamedDecl *> members;
};
/// Owns TU-local records, not AST nodes. Valid only while ASTContext is alive.
/// No MiniMLIR input, object layout inference, or external implementation
/// traversal.
class Registry {
public:
  clang::ASTContext &ast;
  std::map<std::string, TypeRecord> types;
  std::map<std::string, const clang::FunctionDecl *> functions;
  std::map<std::string, const clang::TypedefNameDecl *> aliases;
  std::map<std::string, const clang::CXXRecordDecl *> localForwards;
  std::map<std::string, const clang::UsingShadowDecl *> imports;
  llvm::json::Array uses, relations, unsupported;
  std::set<std::string> issueKeys;
  explicit Registry(clang::ASTContext &a) : ast(a) {}
  /// Stable USR with location/kind/signature fallback; never a short-name key.
  std::string id(const clang::Decl *d) const;
  /// Source provenance for a real declaration; does not copy macro text.
  llvm::json::Object source(const clang::Decl *d) const;
  bool external(const clang::Decl *d) const;
  /// Recursively register signature types; pointer/reference edges need no
  /// layout. Value types upgrade to Complete. Cycles terminate via
  /// identity/state records.
  void need(clang::QualType t, bool complete, llvm::StringRef reason);
  /// Add a selected interface to its actual receiver or namespace. Returns
  /// nothing. Templates are recorded, but unsupported dependent definitions are
  /// not invented.
  void interface(const clang::FunctionDecl *f,
                 const clang::CXXRecordDecl *receiver, clang::SourceLocation at,
                 llvm::StringRef operation);
  void issue(const clang::Decl *d, llvm::StringRef category,
             llvm::StringRef why);
  /// Serialize facts and explicit generation boundaries; compilation is
  /// separate.
  llvm::json::Object report() const;
};
} // namespace buddy::extract
