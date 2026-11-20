"""Move an audited nested record definition into its owner, preserving identity."""
import hashlib
import re

FILE = "mlir/IR/ValueRange.h"
BODY_SHA256 = "b93376996c186c074e6f0c29fc0bed18de45cfe5470a7aaa5a5b72519d016347"
BODY = """class ResultRange::UseIterator final
    : public llvm::iterator_facade_base<UseIterator, std::forward_iterator_tag, OpOperand> {
public:
  explicit UseIterator(ResultRange results, bool end = false);
  UseIterator &operator++();
  OpOperand &operator*() const;
  bool operator!=(const UseIterator &rhs) const;
private:
  ResultRange::iterator it, endIt;
  Value::use_iterator use;
};"""


def audited_slice(body: str) -> bool:
    """Accept the audited layout with any subset of removable ordinary methods.

    Member extraction may remove these declarations. Constructors, fields,
    bases and aliases must remain identical; the relocation preserves the
    selected text rather than restoring discarded methods.
    """
    if hashlib.sha256(BODY.encode()).hexdigest() != BODY_SHA256:
        raise ValueError("Invalid audited iterator reference")
    expected = BODY
    for declaration in ("UseIterator &operator++();", "OpOperand &operator*() const;",
                        "bool operator!=(const UseIterator &rhs) const;"):
        if body.count(declaration) > 1:
            return False
        body = body.replace(declaration, "")
        expected = expected.replace(declaration, "")
    return re.sub(r"\s+", "", body) == re.sub(r"\s+", "", expected)


def inline_use_iterator(headers: dict[str, str]) -> list[dict]:
    """Avoid Frama-Clang's lexical/semantic context assertion for UseIterator.

    Args: headers: in-memory conversion-copy headers, not the canonical library.
    Returns: deletion then insertion records using sequential byte coordinates.
    Raises: ValueError if the retained class/forward declaration shape differs.
    The same ResultRange::UseIterator, fields, bases and signatures are retained;
    no iterator implementation is introduced.
    """
    text = headers.get(FILE, "")
    marker = "class ResultRange::UseIterator final"
    if marker not in text:
        return []
    begin = text.index(marker)
    end = text.index("\n};", begin) + len("\n};")
    original = text[begin:end]
    if not audited_slice(original):
        raise ValueError("ResultRange::UseIterator changed; audit relocation")
    forward = "  class UseIterator;"
    shortened = text[:begin] + text[end:]
    if shortened.count(forward) != 1:
        raise ValueError("UseIterator owner forward declaration is not unique")
    at = shortened.index(forward)
    body = original.replace(marker, "class UseIterator final", 1)
    body = "\n".join("  " + line for line in body.splitlines())
    headers[FILE] = shortened[:at] + body + shortened[at+len(forward):]
    return [
        {"rule": "nested-use-iterator-relocation", "file": FILE,
         "offset": len(text[:begin].encode()), "length": len(original.encode()),
         "before": original, "after": "",
         "reason": "Remove the out-of-line nested definition that triggers ClangVisitor.cpp:8370"},
        {"rule": "nested-use-iterator-relocation", "file": FILE,
         "offset": len(shortened[:at].encode()), "length": len(forward.encode()),
         "before": forward, "after": body,
         "reason": "Define the same nested type inside its original owner without changing layout or members"},
    ]
