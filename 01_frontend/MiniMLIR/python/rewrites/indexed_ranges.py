"""Expose concrete range/nested iterator definitions to Frama-Clang 0.0.20."""
import hashlib
import re
from .typed_attr import TOKENS

SOURCE = "llvm/ADT/STLExtras.h"
# Ordinary members may have been deleted by declaration extraction. All layout,
# bases, special members and constructor declarations must match this skeleton.
REMOVABLE = ("ReferenceT operator[](long index) const;", "iterator begin() const;",
             "iterator end() const;", "ReferenceT operator*() const;",
             "ReferenceT front() const;")
SKELETON_SHA256 = "976f73a1fed0fdbc05d5a6106d448da1d57aec2c7b510c6953f5c772019b991f"
TARGETS = (
    ("mlir/IR/ValueRange.h", "ValueRange", "class ValueRange final",
     "llvm::PointerUnion<const ::mlir::Value *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Value> *>", "::mlir::Value"),
    ("mlir/IR/TypeRange.h", "TypeRange", "class TypeRange :",
     "llvm::PointerUnion<const ::mlir::Value *, const ::mlir::Type *, ::mlir::OpOperand *, ::mlir::detail::OpResultImpl *, const llvm::Repeated<::mlir::Type> *, const llvm::Repeated<::mlir::Value> *>", "::mlir::Type"),
    ("mlir/IR/ValueRange.h", "ResultRange", "class ResultRange final",
     "::mlir::detail::OpResultImpl *", "::mlir::OpResult"),
)


def skeleton(text: str) -> str:
    """Normalize only whitespace and audited removable method declarations."""
    for method in REMOVABLE:
        if text.count(method) > 1:
            raise ValueError("Duplicate indexed range member")
        text = text.replace(method, "")
    return re.sub(r"\s+", "", text)


def specialize_indexed_ranges(headers: dict[str, str]) -> list[dict]:
    """Clone selected source templates with exact concrete parameter types.

    Args: headers: mutable conversion include tree after member extraction.
    Returns: insertion records identifying source template and substitutions.
    Raises: ValueError on structural/template changes requiring another audit.
    Type identity, nested iterator bases, storage and member signatures remain
    unchanged. This adjusts template emission order, not iteration algorithms.
    """
    source = headers.get(SOURCE, "")
    marker = "class indexed_accessor_range_base {"
    if marker not in source:
        return []
    begin = source.index(marker)
    end = source.index("\n};", begin) + len("\n};")
    body = source[begin:end]
    if hashlib.sha256(skeleton(body).encode()).hexdigest() != SKELETON_SHA256:
        raise ValueError("Indexed range template changed; audit concrete expansion")
    edits = []
    for file, name, anchor, owner, value in TARGETS:
        text = headers.get(file, "")
        guard = "// Concrete indexed range: " + name
        if anchor not in text or guard in text:
            continue
        args = {"DerivedT": "::mlir::" + name, "BaseT": owner,
                "T": value, "PointerT": value, "ReferenceT": value}
        expanded = TOKENS.sub(lambda match: args.get(match.group(), match.group()), body)
        expanded = expanded.replace(marker, "class indexed_accessor_range_base<" +
                                    ", ".join(args.values()) + "> {", 1)
        addition = (guard + "\nclass " + name + ";\n} // namespace mlir\n"
                    "namespace llvm { namespace detail {\ntemplate<>\n" + expanded +
                    "\n} }\nnamespace mlir {\n")
        at = text.index(anchor)
        headers[file] = text[:at] + addition + text[at:]
        edits.append({"rule": "indexed-range-concrete-expansion", "file": file,
                      "offset": len(text[:at].encode()), "length": 0,
                      "before": "", "after": addition, "template_source": SOURCE,
                      "template_sha256": hashlib.sha256(body.encode()).hexdigest(),
                      "arguments": args,
                      "reason": "Emit the same concrete nested iterator before iterator_range stores it by value"})
    return edits
