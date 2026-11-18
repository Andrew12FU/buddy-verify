"""Preserve void and WalkResult paths without return(void-expression)."""
import hashlib
import re

TARGETS = (('mlir/IR/Visitors.h', 'Block *>::value,', '', '\n}', 'f0e74f4d634ebab4d2db1efdffdfa11a8390638e4c6f1b1954eb7ad6f7308d99'), ('mlir/IR/Operation.h', 'num_args == 1,', '  ', '\n  }', '6ac07e4421d17bccab962976034c624c0ae0bf3307404b8e658ca2d9eb496b91'), ('mlir/IR/OpDefinition.h', 'num_args == 1,', '  ', '\n  }', '7280761f9a89cbdd90140474d0997799e49c4311939be6051ed3c299390bb283'))


def expand_walk_returns(headers: dict[str, str]) -> list[dict]:
    """Split each audited wrapper using the original predicate plus RetT.

    Args: headers: conversion-copy source text, changed in memory only.
    Returns: sequential replacements, or none for absent/previously split wrappers.
    Raises: ValueError on a changed wrapper shape, preventing broad matching.
    Callback bodies and actual traversal calls are retained in both paths.
    """
    edits = []
    for file, marker, indent, ending, expected in TARGETS:
        text = headers.get(file, "")
        if marker not in text:
            continue
        at = text.index(marker)
        begin = text.rindex(indent + "template <", 0, at)
        end = text.index(ending, text.index("walk(", at)) + len(ending)
        original = text[begin:end]
        if hashlib.sha256(original.encode()).hexdigest() != expected:
            raise ValueError("Walk wrapper changed; audit required: " + file)
        statement = re.search(r"\n(?P<indent> +)return (?P<expression>[\s\S]*?);\n", original)
        if not statement:
            raise ValueError("Expected one return expression in " + file)
        void = original.replace(marker, marker[:-1] + " && std::is_same<RetT, void>::value,")
        void = void.replace(statement.group(), "\n" + statement["indent"] + statement["expression"] +
                            ";\n" + statement["indent"] + "return;\n")
        value = original.replace(marker, marker[:-1] + " && !std::is_same<RetT, void>::value,")
        replacement = void + "\n" + value
        headers[file] = text[:begin] + replacement + text[end:]
        edits.append({"rule": "walk-void-return-expansion", "file": file,
                      "offset": len(text[:begin].encode()), "length": len(original.encode()),
                      "before": original, "after": replacement,
                      "reason": "Sequence void traversal and preserve the exact nonvoid return; original overload predicate remains"})
    return edits
