"""仅在转换用副本中展开已触发的不支持继承构造，保留约束与默认构造。"""

import hashlib

from .typed_attr import ATTRIBUTES, INTERFACE

TOSA = "mlir/Dialect/Tosa/IR/TosaOps.h"
OP_DEFINITION = "mlir/IR/OpDefinition.h"
TYPED_ANCHOR = "      TypedAttr, detail::TypedAttrInterfaceTraits>::AttributeInterface;"
TYPED_PROTOTYPE = """  template <typename T,
            std::enable_if_t<std::is_base_of<Trait<T>, T>::value> * = nullptr>
  Interface(T t);"""
SHAPES = {
    "TypedAttr": "39013508a33765a59fafcad5455bdbcaeb79ce2e41b69260c6ccc959b398440f",
    "SigmoidOp": "06f61fa8412fe8d329197d34fba6e3441ae06bf082aa5b546b76734aa51a3031",
}


def _insert(headers: dict[str, str], file: str, owner: str, anchor: str,
            addition: str, rule: str, prototype_source: str, prototype: str) -> list[dict]:
    """Check a known class body and insert one declaration block, or skip it.

    Offsets refer to the current header after previous rule edits. A class not
    retained by extraction is skipped; an altered class requires a fresh audit.
    """
    text = headers.get(file, "")
    marker = "class " + owner + "\n"
    if marker not in text:
        return []
    if addition in text:
        if text.count(addition) != 1:
            raise ValueError(f"Duplicate compatibility constructors for {owner}")
        return []
    begin = text.index(marker)
    end = text.index("\n};", begin) + len("\n};")
    body = text[begin:end]
    if hashlib.sha256(body.encode()).hexdigest() != SHAPES[owner] or body.count(anchor) != 1:
        raise ValueError(f"{owner} constructor shape changed; audit expansion before reuse")
    offset = begin + body.index(anchor) + len(anchor)
    headers[file] = text[:offset] + addition + text[offset:]
    return [{"rule": rule, "file": file, "offset": len(text[:offset].encode()),
             "length": 0, "before": "", "after": addition,
             "prototype_source": prototype_source, "prototype": prototype,
             "owner": "mlir::" + ("tosa::" if owner == "SigmoidOp" else "") + owner,
             "reason": "Explicit constructor declaration avoids CXXInheritedCtorInitExpr in the conversion model",
             "abstraction_effect": "Construction remains externally declared; no implementation or IR behavior is synthesized"}]


def expand(headers: dict[str, str]) -> list[dict]:
    """Expand TypedAttr's trait-constrained constructor and SigmoidOp(nullptr).

    Args:
        headers: In-memory conversion view, following the Interface specialization.
    Returns:
        Ordered declaration insertions. Inherited overloads remain available;
        defaulted constructors restore implicit defaults suppressed by new ones.
    Raises:
        ValueError: Prototype or class shape differs from the audited model.
    """
    edits = []
    # SiLU passes FloatAttr; Depthwise also passes IntegerAttr from getIndexAttr.
    # Both use the same inherited, trait-constrained constructor. A forward
    # declaration alone does not select this adaptation.
    if any("class " + name + "\n" in headers.get(ATTRIBUTES, "")
           for name in ("FloatAttr", "IntegerAttr")):
        if headers.get(INTERFACE, "").count(TYPED_PROTOTYPE) != 1:
            raise ValueError("Inherited Interface constructor prototype changed")
        prototype = TYPED_PROTOTYPE.replace("Trait<T>", "InterfaceBase::template Trait<T>")
        prototype = prototype.replace("Interface(T t)", "TypedAttr(T t)")
        addition = ("\n  // Conversion view: preserve the inherited availability constraint.\n"
                    "  TypedAttr() = default;\n" + prototype + "\n")
        edits += _insert(headers, ATTRIBUTES, "TypedAttr", TYPED_ANCHOR, addition,
                         "typed-attr-constructor-expansion", INTERFACE, TYPED_PROTOTYPE)
    if "class SigmoidOp\n" in headers.get(TOSA, ""):
        prototype = "  inline Op(std::nullptr_t);"
        if headers.get(OP_DEFINITION, "").count(prototype) != 1:
            raise ValueError("Inherited Op(nullptr) constructor prototype changed")
        addition = ("\n  // Conversion view: inherited nullptr constructor with its default restored.\n"
                    "  SigmoidOp() = default;\n" + prototype.replace("Op(", "SigmoidOp(") + "\n")
        edits += _insert(headers, TOSA, "SigmoidOp", "  using Op::Op;", addition,
                         "op-nullptr-constructor-expansion", OP_DEFINITION, prototype)
    return edits
