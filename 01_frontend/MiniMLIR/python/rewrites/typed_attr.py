"""有界 TypedAttr 改写：展开所需的 Interface 特化，保留类型身份和接口。

这里只接受 MiniMLIR 已审计模板形状，不是任意 C++ 的模板实例化器。
特化体从输入模板逐 token 替换形参得到，不另行维护第二套接口定义。
"""

import hashlib
import re

INTERFACE = "mlir/Support/InterfaceSupport.h"
ATTRIBUTES = "mlir/IR/BuiltinAttributes.h"
PARAMETERS = """template <typename ConcreteType, typename ValueT, typename Traits,
          typename BaseType,
          template <typename, template <typename> class> class BaseTrait>"""
CLASS_START = "class Interface : public BaseType {"
CLASS_END = "\n};\n} // namespace detail"
TARGET = "class TypedAttr\n"
# A change to the source template requires an explicit rule audit. In
# particular, do not substitute newly shadowed template parameter names.
TEMPLATE_SHA256 = "bdfdd233de3ebcba1d8feddc07c49e4f939a7bfbbf00cdeb86ae60dc0c16f989"
ARGUMENTS = {
    "ConcreteType": "::mlir::TypedAttr",
    "ValueT": "::mlir::Attribute",
    "Traits": "::mlir::detail::TypedAttrInterfaceTraits",
    "BaseType": "::mlir::Attribute",
    "BaseTrait": "::mlir::AttributeTrait::TraitBase",
}
# Comments and literals are indivisible; replacement only applies to identifier
# tokens. The bounded source is checked below before this lexical substitution.
TOKENS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'
                    r"'(?:\\.|[^'\\])*'|[A-Za-z_][A-Za-z_0-9]*")


def specialize(headers: dict[str, str]) -> list[dict]:
    """Expand the TypedAttr Interface specialization in an in-memory copy.

    Args:
        headers: Library-relative header paths to UTF-8 source text. The caller
            owns this copy; canonical files and extracted files remain read-only.
    Returns:
        Applied-edit records with source template, substitutions and inserted text.
        An empty list means TypedAttr was not selected by member extraction.
    Raises:
        ValueError: Selected headers no longer have the audited declaration shape.
    """
    attributes = headers.get(ATTRIBUTES, "")
    if TARGET not in attributes:
        return []
    interface = headers.get(INTERFACE, "")
    signature = PARAMETERS + "\n" + CLASS_START
    if interface.count(signature) != 1 or interface.count(CLASS_END) != 1:
        raise ValueError("Interface template changed; audit header specialization before reuse")
    if attributes.count(TARGET) != 1:
        raise ValueError("TypedAttr definition is not unique")
    begin = interface.index(CLASS_START)
    end = interface.index(CLASS_END, begin) + len("\n};")
    original = interface[begin:end]
    if hashlib.sha256(original.encode()).hexdigest() != TEMPLATE_SHA256:
        raise ValueError("Interface template body changed; audit header specialization before reuse")
    body = TOKENS.sub(lambda m: ARGUMENTS.get(m.group(), m.group()), original)
    argument_list = ", ".join(ARGUMENTS.values())
    body = body.replace("class Interface :", f"class Interface<{argument_list}> :", 1)
    inserted = ("// Converter compatibility: exact specialization of the retained template.\n"
                "// See header-rewrite.json for source and parameter substitutions.\n"
                "class TypedAttr;\nnamespace detail {\ntemplate <>\n" + body +
                "\n} // namespace detail\n"
                "template class AttributeInterface<TypedAttr, detail::TypedAttrInterfaceTraits>;\n")
    if inserted in attributes:
        if attributes.count(inserted) != 1:
            raise ValueError("Duplicate TypedAttr compatibility specializations")
        return []
    offset = attributes.index(TARGET)
    headers[ATTRIBUTES] = attributes[:offset] + inserted + attributes[offset:]
    return [{"rule": "typed-attr-interface-specialization", "file": ATTRIBUTES,
             "offset": len(attributes[:offset].encode()), "length": 0, "before": "", "after": inserted,
             "template_source": INTERFACE, "template_begin": len(interface[:begin].encode()),
             "template_end": len(interface[:end].encode()),
             "arguments": ARGUMENTS,
             "reason": "Expose nested Trait definitions to Frama-Clang 0.0.20（Clang 18.1.8） without changing their qualified type identity"}]
