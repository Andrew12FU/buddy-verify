"""Expand only newly audited interface specializations in the conversion copy."""
import hashlib
from .typed_attr import INTERFACE, CLASS_START, CLASS_END, TEMPLATE_SHA256, TOKENS

# Header, concrete interface, wrapper and common value base. Trait identities
# follow local generated MLIR headers; no standalone replacement class is used.
TARGETS = (
    ("mlir/IR/Attributes.h", "OpAsmAttrInterface", "AttributeInterface", "Attribute", "AttributeTrait"),
    ("mlir/IR/Attributes.h", "BlobAttr", "AttributeInterface", "Attribute", "AttributeTrait"),
    ("mlir/IR/OpDefinition.h", "VectorTransferOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "LoopLikeOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "MaskableOpInterface", "OpInterface", "Operation *", "OpTrait", "vector"),
    ("mlir/IR/BuiltinAttributes.h", "ElementsAttr", "AttributeInterface", "Attribute", "AttributeTrait"),
    ("mlir/IR/Attributes.h", "MemRefLayoutAttrInterface", "AttributeInterface", "Attribute", "AttributeTrait"),
    ("mlir/IR/BuiltinTypes.h", "PtrLikeTypeInterface", "TypeInterface", "Type", "TypeTrait"),
    ("mlir/Interfaces/CallInterfaces.h", "CallOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/Interfaces/ControlFlowInterfaces.h", "BranchOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/Interfaces/ControlFlowInterfaces.h", "RegionBranchOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "ViewLikeOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "MemorySpaceCastConsumerOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "OffsetSizeAndStrideOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "RegionBranchTerminatorOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "CastOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "AlignmentAttrOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "InferIntRangeInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/IR/OpDefinition.h", "SymbolUserOpInterface", "OpInterface", "Operation *", "OpTrait"),
    ("mlir/Dialect/GPU/IR/GPUDialect.h", "AsyncOpInterface", "OpInterface", "Operation *", "OpTrait", "gpu"),
)

def specialize_bounded(headers: dict[str, str]) -> list[dict]:
    """Clone the hash-checked Interface template for selected concrete APIs.

    Args: headers: mutable conversion-copy text, never the canonical files.
    Returns: ordered insertions with template source and parameter substitutions.
    Raises: ValueError if the common audited template shape has changed.
    """
    edits = []
    for target in TARGETS:
        file, name, wrapper, value, trait = target[:5]
        scope = target[5] if len(target) == 6 else ""
        qualified = (scope + "::" if scope else "") + name
        trait_scope = (scope + "::" if scope else "") + "detail::"
        text = headers.get(file, "")
        marker = "class " + name + " :"
        if marker not in text:
            continue
        interface = headers[INTERFACE]
        begin = interface.index(CLASS_START)
        end = interface.index(CLASS_END, begin) + len("\n};")
        original = interface[begin:end]
        if hashlib.sha256(original.encode()).hexdigest() != TEMPLATE_SHA256:
            raise ValueError("Common Interface template changed; audit required")
        base = "::mlir::OpState" if wrapper == "OpInterface" else "::mlir::" + value
        arguments = {"ConcreteType": "::mlir::" + qualified,
                     "ValueT": "::mlir::" + value,
                     "Traits": "::mlir::" + trait_scope + name + "InterfaceTraits",
                     "BaseType": base, "BaseTrait": "::mlir::" + trait + "::TraitBase"}
        body = TOKENS.sub(lambda match: arguments.get(match.group(), match.group()), original)
        body = body.replace("::mlir::Operation *()", "ValueType()")
        body = body.replace("class Interface :", "class Interface<" + ", ".join(arguments.values()) + "> :", 1)
        addition = ("class " + name + ";\nnamespace detail {\ntemplate <>\n" + body +
                    "\n}\ntemplate class " + wrapper + "<" + name + ", detail::" + name + "InterfaceTraits>;\n")
        if addition in text:
            continue
        if scope:
            # The Interface primary is in mlir::detail, while the concrete
            # interface/traits live in mlir::gpu. Insert outside that inner
            # namespace; defining gpu::detail::Interface would change identity.
            marker = "namespace mlir { namespace " + scope + " {\n" + marker
            addition = ("namespace mlir {\nnamespace " + scope + " { class " + name + "; }\n"
                        "namespace detail {\ntemplate <>\n" + body + "\n}\n"
                        "template class " + wrapper + "<" + qualified + ", " +
                        trait_scope + name + "InterfaceTraits>;\n}\n")
        if addition in text:
            if text.count(addition) != 1:
                raise ValueError("Duplicate compatibility specialization for " + qualified)
            continue
        offset = text.index(marker)
        headers[file] = text[:offset] + addition + text[offset:]
        # Match only the audited inherited-constructor line in this interface.
        # Keep using-declarations and the canonical library unchanged.
        anchor = "  using " + wrapper + "<" + name + ", detail::" + name + "InterfaceTraits>::" + wrapper + ";"
        constructor = ("\n  " + name + "() = default;\n"
                       "  template<typename T, std::enable_if_t<std::is_base_of<InterfaceBase::template Trait<T>, T>::value> * = nullptr>\n  " + name + "(T t);\n")
        current = headers[file]
        if name == "MemRefLayoutAttrInterface" and constructor not in current:
            if current.count(anchor) != 1:
                raise ValueError("Inherited constructor shape changed for " + name)
            at = current.index(anchor) + len(anchor)
            headers[file] = current[:at] + constructor + current[at:]
            ctor_edit = {"rule": "bounded-interface-constructor-expansion", "file": file,
                         "offset": len(current[:at].encode()), "length": 0,
                         "before": "", "after": constructor,
                         "owner": "mlir::" + name,
                         "reason": "Expand inherited constructor with its original trait availability constraint"}
        else:
            ctor_edit = None
        edits.append({"rule": "bounded-interface-specialization", "file": file,
                      "offset": len(text[:offset].encode()), "length": 0,
                      "before": "", "after": addition, "arguments": arguments,
                      "template_source": INTERFACE,
                      "reason": "Preserve interface identity while exposing nested Trait before instantiation"})
        if ctor_edit:
            edits.append(ctor_edit)
    file = "mlir/IR/OpDefinition.h"
    text = headers.get(file, "")
    anchor = "class OpFoldResult : public llvm::PointerUnion<Attribute, Value> {\npublic:\n  using PointerUnion::PointerUnion;"
    addition = "\n  OpFoldResult() = default;\n  OpFoldResult(Attribute attribute);\n  OpFoldResult(Value value);"
    if anchor in text and addition not in text:
        at = text.index(anchor) + len(anchor)
        headers[file] = text[:at] + addition + text[at:]
        edits.append({"rule": "op-fold-result-constructor-expansion", "file": file,
                      "offset": len(text[:at].encode()), "length": 0,
                      "before": "", "after": addition,
                      "reason": "Expand exactly the Attribute/Value alternatives of the inherited PointerUnion constructor"})
    file = "mlir/IR/BuiltinTypes.h"
    text = headers.get(file, "")
    marker = "class MemRefType : public Type::TypeBase<MemRefType, BaseMemRefType, detail::MemRefTypeStorage, PtrLikeTypeInterface::Trait, ShapedType::Trait>"
    addition = "class MemRefType;\ntemplate class detail::StorageUserBase<MemRefType, BaseMemRefType, detail::MemRefTypeStorage, detail::TypeUniquer, PtrLikeTypeInterface::Trait, ShapedType::Trait>;\n"
    if marker in text and addition not in text:
        at = text.index(marker)
        headers[file] = text[:at] + addition + text[at:]
        edits.append({"rule": "memref-storage-base-instantiation", "file": file,
                      "offset": len(text[:at].encode()), "length": 0,
                      "before": "", "after": addition,
                      "reason": "Generate the complete base specialization before the concrete MemRefType"})
    return edits
