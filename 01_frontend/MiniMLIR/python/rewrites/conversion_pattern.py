"""Expose the inherited ConversionPattern constructor in the conversion copy."""

FILE = "mlir/Transforms/DialectConversion.h"
ANCHOR = " using RewritePattern::RewritePattern;"
ADDITION = "\n ConversionPattern(StringRef rootName,PatternBenefit benefit,MLIRContext *context,ArrayRef<StringRef> generatedNames = {});"


def expand_conversion_pattern(headers: dict[str, str]) -> list[dict]:
    """Declare the selected root-name constructor without an inherited-init body.

    Args: headers: mutable, extracted conversion view; canonical text is untouched.
    Returns: one insertion, or none if the class is absent/already adapted.
    Raises: ValueError if the retained inherited-constructor anchor is ambiguous.
    The signature/default are those of Pattern; external construction semantics
    remain to be specified. Other inherited overloads stay available.
    """
    text = headers.get(FILE, "")
    if "class ConversionPattern :" not in text or ADDITION in text:
        return []
    if text.count(ANCHOR) != 1:
        raise ValueError("ConversionPattern inherited constructor changed")
    at = text.index(ANCHOR) + len(ANCHOR)
    headers[FILE] = text[:at] + ADDITION + text[at:]
    return [{"rule": "conversion-pattern-constructor-expansion", "file": FILE,
             "offset": len(text[:at].encode()), "length": 0,
             "before": "", "after": ADDITION,
             "prototype_source": "mlir/IR/PatternMatch.h",
             "reason": "Preserve the root-name constructor signature while avoiding inherited-init expressions"}]
