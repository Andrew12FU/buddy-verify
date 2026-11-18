#!/usr/bin/env python3
"""共享头文件兼容改写的类型关系、约束、原件保护和失配拒绝回归。"""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

FRONTEND = Path(__file__).resolve().parents[1]
MINI = FRONTEND.parent / "MiniMLIR"
sys.path.insert(0, str(FRONTEND))
sys.path.insert(0, str(MINI))
from experiment.toolchain import compiler_environment
from python.emission import digest
from python.rewrites.pipeline import rewrite_headers
from python.rewrites.constructors import expand, TOSA
from python.rewrites.typed_attr import ATTRIBUTES, INTERFACE, specialize

PARSER = argparse.ArgumentParser()
PARSER.add_argument("--clang", type=Path, required=True)
ARGS, REST = PARSER.parse_known_args()
ENV = compiler_environment(ARGS.clang.absolute().parents[1])

CONTRACT = """#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
using namespace mlir;
using I = detail::Interface<TypedAttr, Attribute, detail::TypedAttrInterfaceTraits,
                            Attribute, AttributeTrait::TraitBase>;
struct Unrelated {};
static_assert(std::is_base_of<Attribute, TypedAttr>::value);
static_assert(std::is_base_of<I::Trait<FloatAttr>, FloatAttr>::value);
static_assert(std::is_base_of<I::Trait<IntegerAttr>, IntegerAttr>::value);
static_assert(std::is_same<I::Trait<FloatAttr>::ModelT,
                          detail::TypedAttrInterfaceTraits::Model<FloatAttr>>::value);
static_assert(std::is_constructible<TypedAttr, FloatAttr>::value);
static_assert(std::is_convertible<FloatAttr, TypedAttr>::value);
static_assert(std::is_constructible<TypedAttr>::value);
static_assert(std::is_constructible<TypedAttr, const TypedAttr &>::value);
static_assert(std::is_trivially_copy_constructible<TypedAttr>::value);
static_assert(!std::is_constructible<TypedAttr, Unrelated>::value);
static_assert(!std::is_convertible<Attribute, TypedAttr>::value);
static_assert(sizeof(TypedAttr) == sizeof(Attribute) + sizeof(void*));
static_assert(std::is_constructible<tosa::SigmoidOp>::value);
static_assert(std::is_constructible<tosa::SigmoidOp, std::nullptr_t>::value);
static_assert(std::is_convertible<std::nullptr_t, tosa::SigmoidOp>::value);
static_assert(std::is_constructible<tosa::SigmoidOp, Operation *>::value);
static_assert(!std::is_convertible<Operation *, tosa::SigmoidOp>::value);
"""

CALLBACK_CONTRACT = """#include "llvm/ADT/STLFunctionalExtras.h"
struct Builder {};
struct Location {};
struct Values {};
void good(Builder &, Location, Values);
int withResult(Builder &, Location, Values);
void wrongReference(Builder *, Location, Values);
void wrongArity(Builder &, Location);
struct NonCallable {};
using Ref = llvm::function_ref<void(Builder &, Location, Values)>;
using IntRef = llvm::function_ref<int(Builder &, Location, Values)>;
static_assert(std::is_constructible<Ref>::value);
static_assert(std::is_constructible<Ref, std::nullptr_t>::value);
static_assert(std::is_convertible<std::nullptr_t, Ref>::value);
static_assert(std::is_constructible<Ref, const Ref &>::value);
static_assert(std::is_constructible<Ref, decltype(good) &>::value);
static_assert(std::is_convertible<decltype(good) &, Ref>::value);
static_assert(std::is_constructible<Ref, decltype(withResult) &>::value);
static_assert(!std::is_constructible<Ref, decltype(wrongReference) &>::value);
static_assert(!std::is_constructible<Ref, decltype(wrongArity) &>::value);
static_assert(!std::is_constructible<Ref, NonCallable>::value);
static_assert(std::is_constructible<IntRef, decltype(withResult) &>::value);
static_assert(!std::is_constructible<IntRef, decltype(good) &>::value);
static_assert(!std::is_convertible<Ref, bool>::value);
static_assert(std::is_same<decltype(std::declval<Ref>()(
    std::declval<Builder &>(), std::declval<Location>(), std::declval<Values>())),
    void>::value);
static_assert(std::is_same<decltype(std::declval<IntRef>()(
    std::declval<Builder &>(), std::declval<Location>(), std::declval<Values>())),
    int>::value);
void localClosure(Builder &builder) {
  auto callback = [&builder](Builder &, Location, Values) { (void)builder; };
  static_assert(std::is_constructible<Ref, decltype(callback) &>::value);
  Ref view(callback);
}
"""


class HeaderRewriteTests(unittest.TestCase):
    def setUp(self):
        """每轮复制完整测试库；生产运行仍从 AST 专用库开始。"""
        temporary = tempfile.TemporaryDirectory(prefix="buddy-mini-headers-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / "include"
        shutil.copytree(MINI / "include", self.source)
        self.manifest = self.root / "interface-manifest.json"
        self.hashes = {str(p.relative_to(self.source)): digest(p)
                       for p in sorted(self.source.rglob("*")) if p.is_file()}
        self.manifest.write_text(json.dumps({"library_root": str(MINI / "include"),
                                            "generated_headers_sha256": self.hashes}))
        self.output = self.root / "conversion-include"

    def compile(self, include: Path, text: str):
        """让实际 C++ 编译器裁决类型身份和重载约束，失败保留诊断。"""
        source = self.root / "contract.cpp"
        source.write_text(text)
        result = subprocess.run([str(ARGS.clang), "-std=c++17", "-nostdinc++",
                                 "-I" + str(include), "-fsyntax-only", str(source)],
                                env=ENV, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_preserves_type_relationships_and_constructor_constraints(self):
        self.compile(self.source, CONTRACT)
        rewrite_headers(self.source, self.manifest, self.output)
        self.compile(self.output, CONTRACT)
        self.assertEqual(self.hashes, {str(p.relative_to(self.source)): digest(p)
                                      for p in self.source.rglob("*") if p.is_file()})

    def test_external_callback_preserves_signature_and_constraints(self):
        """接口抽象仍拒绝错参/错返回值；局部闭包可实例化，不要求外部实现。"""
        self.compile(self.source, CALLBACK_CONTRACT)
        rewrite_headers(self.source, self.manifest, self.output)
        self.compile(self.output, CALLBACK_CONTRACT)

    def test_transpose_interfaces_keep_traits_and_range_types(self):
        """新增接口在共享库与转换副本中具有相同基类和调用类型。"""
        contract = r'''
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
using namespace mlir;
using Parallel = affine::AffineParallelOp;
static_assert(std::is_base_of<OpTrait::OneRegion<Parallel>, Parallel>::value);
static_assert(std::is_base_of<vector::MaskableOpInterface::Trait<vector::TransferReadOp>, vector::TransferReadOp>::value);
static_assert(std::is_base_of<Attribute, DenseI64ArrayAttr>::value);
static_assert(std::is_same<decltype(std::declval<ValueRange>().getTypes()),ValueTypeRange<ValueRange>>::value);
static_assert(std::is_same<decltype(std::declval<DenseI64ArrayAttr>().asArrayRef()),ArrayRef<int64_t>>::value);
using Callback=llvm::function_ref<void(OpBuilder &,Location,ValueRange)>;
using Build=void (*)(OpBuilder &,Location,ValueRange,ValueRange,ArrayRef<int64_t>,Callback);
Build build=static_cast<Build>(&affine::buildAffineLoopNest);
void projection() {
 auto values=llvm::to_vector<4>(llvm::map_range(ArrayRef<affine::LoopReduction>{},
     [](const affine::LoopReduction &r){return r.value;}));
 static_assert(std::is_same<decltype(values),SmallVector<Value,4>>::value);
}
'''
        self.compile(self.source, contract)
        rewrite_headers(self.source, self.manifest, self.output)
        self.compile(self.output, contract)

    def test_recorded_byte_edits_reproduce_generated_header(self):
        report = rewrite_headers(self.source, self.manifest, self.output)
        data = {name: (self.source / name).read_bytes()
                for name in report["input_headers_sha256"]}
        for edit in report["edits"]:
            name = edit["file"]
            begin, end = edit["offset"], edit["offset"] + edit["length"]
            self.assertEqual(data[name][begin:end], edit["before"].encode())
            data[name] = data[name][:begin] + edit["after"].encode() + data[name][end:]
        for name, content in data.items():
            self.assertEqual(content, (self.output / name).read_bytes())

    def test_changed_template_constraint_is_rejected(self):
        headers = {INTERFACE: (self.source / INTERFACE).read_text(),
                   ATTRIBUTES: (self.source / ATTRIBUTES).read_text()}
        headers[INTERFACE] = headers[INTERFACE].replace(
            "std::is_base_of<Trait<T>, T>::value", "true")
        with self.assertRaisesRegex(ValueError, "template body changed"):
            specialize(headers)

    def test_no_typed_attr_definition_needs_no_specialization(self):
        headers = {ATTRIBUTES: "namespace mlir { class TypedAttr; }\n"}
        self.assertEqual(specialize(headers), [])

    def test_specialization_is_idempotent(self):
        headers = {INTERFACE: (self.source / INTERFACE).read_text(),
                   ATTRIBUTES: (self.source / ATTRIBUTES).read_text()}
        self.assertEqual(len(specialize(headers)), 1)
        once = dict(headers)
        self.assertEqual(specialize(headers), [])
        self.assertEqual(headers, once)

    def test_constructor_expansion_is_idempotent(self):
        headers = {str(p.relative_to(self.source)): p.read_text()
                   for p in self.source.rglob("*") if p.is_file()}
        specialize(headers)
        self.assertEqual(len(expand(headers)), 2)
        once = dict(headers)
        self.assertEqual(expand(headers), [])
        self.assertEqual(headers, once)

    def test_changed_op_constructor_prototype_is_rejected(self):
        headers = {str(p.relative_to(self.source)): p.read_text()
                   for p in self.source.rglob("*") if p.is_file()}
        headers["mlir/IR/OpDefinition.h"] = headers["mlir/IR/OpDefinition.h"].replace(
            "inline Op(std::nullptr_t)", "inline explicit Op(std::nullptr_t)")
        with self.assertRaisesRegex(ValueError, "Op\\(nullptr\\) constructor prototype changed"):
            expand(headers)

    def test_changed_extracted_headers_are_rejected_without_output(self):
        with (self.source / ATTRIBUTES).open("a") as stream:
            stream.write("\nstruct Unexpected {};\n")
        with self.assertRaisesRegex(ValueError, "changed since materialization"):
            rewrite_headers(self.source, self.manifest, self.output)
        self.assertFalse(self.output.exists())

    def test_historical_report_cannot_be_overwritten(self):
        rewrite_headers(self.source, self.manifest, self.output)
        with self.assertRaisesRegex(ValueError, "reports cannot be overwritten"):
            rewrite_headers(self.source, self.manifest, self.root / "another-include")


if __name__ == "__main__":
    unittest.main(argv=[__file__, *REST])
