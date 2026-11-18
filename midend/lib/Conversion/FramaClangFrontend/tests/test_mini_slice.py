#!/usr/bin/env python3
"""Meaningful slicing boundaries: body/field closure, overloads and templates."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT.parent / "MiniMLIR"))
from experiment.toolchain import compiler_environment, resource_directory
from python.emission import materialize

PARSER = argparse.ArgumentParser()
PARSER.add_argument("--tool", type=Path, required=True)
PARSER.add_argument("--clang", type=Path, required=True)
ARGS, REST = PARSER.parse_known_args()
sys.argv = [sys.argv[0], *REST]
LLVM = ARGS.clang.absolute().parents[1]
ENV = compiler_environment(LLVM)
RESOURCE = str(resource_directory(LLVM))


class MiniSliceTests(unittest.TestCase):
    """Exercise generated C++ behavior, not just planned deletion counts."""

    def slice(self, header: str, program: str, *, mutate: bool = False):
        temporary = tempfile.TemporaryDirectory(prefix="buddy-mini-slice-")
        self.addCleanup(temporary.cleanup)
        directory = Path(temporary.name)
        canonical = directory / "canonical"
        canonical.mkdir()
        (canonical / "api.h").write_text("#pragma once\n" + header)
        source = directory / "pass.cpp"
        source.write_text('#include "api.h"\n' + program)
        plan = directory / "plan.json"
        result = subprocess.run([str(ARGS.tool), str(source), "--library-root", str(canonical),
                                 "--report", str(plan), "--", "-std=c++17", "-I" + str(canonical),
                                 "-resource-dir=" + RESOURCE],
                                env=ENV, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        output = directory / "generated" / "include"
        if mutate:
            with (canonical / "api.h").open("a") as stream:
                stream.write("\nstruct NewlyAdded {};\n")
            with self.assertRaisesRegex(ValueError, "changed since AST extraction"):
                materialize(canonical, source, plan, output)
            self.assertFalse(output.exists())
            return None
        manifest = materialize(canonical, source, plan, output)
        result = subprocess.run([str(ARGS.clang), "-std=c++17", "-fsyntax-only", "-I" + str(output), str(source)],
                                env=ENV, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        return (output / "api.h").read_text(), manifest

    def test_range_hidden_calls_and_partial_specialization_members(self):
        text, _ = self.slice("""
struct Tag {};
struct Iterator { int *p; int &operator*() const; Iterator &operator++();
  bool operator!=(Iterator) const; };
template<typename T, typename U> struct Range;
template<typename T> struct Range<Tag, T> {
  Iterator begin(); Iterator end(); int unused() const;
};
""", "int inspect(Range<Tag, int> &r) { int sum=0; for (int v:r) sum+=v; return sum; }")
        self.assertIn("Iterator end()", text)
        self.assertIn("operator++", text)
        self.assertIn("operator*", text)
        self.assertNotIn("unused()", text)

    def test_tuple_protocol_and_declaration_only_function_template(self):
        text, _ = self.slice("""
namespace api { struct Pair { int a,b; };
template<unsigned I> int get(Pair &&); }
namespace std {
template<typename T> struct tuple_size;
template<unsigned I, typename T> struct tuple_element;
template<> struct tuple_size<api::Pair> { static constexpr unsigned value=2; };
template<unsigned I> struct tuple_element<I, api::Pair> { using type=int; };
}
""", "int inspect(api::Pair p) { auto [a,b]=p; return a+b; }")
        self.assertIn("int get(Pair &&)", text)
        self.assertIn("tuple_size<api::Pair>", text)

    def test_body_private_field_and_overloads(self):
        text, manifest = self.slice("""
struct Token { int value; };
using Number = int;
struct Handle {
  Handle() = default;
  Number used() const { return helper(state); }
  int used(int) const;
  int unused() const;
private:
  Token state;
  static Number helper(const Token &t) { return t.value; }
};
struct Unused { int unrelated; };
""", "int inspect(const Handle &h) { return h.used(); }")
        for needed in ("Token state", "helper(state)", "helper(const Token", "int used(int)", "using Number"):
            self.assertIn(needed, text)
        self.assertNotIn("unused()", text)
        self.assertNotIn("struct Unused", text)
        self.assertTrue(manifest["removed"])

    def test_retained_friend_keeps_namespace_function_at_same_range(self):
        text, _ = self.slice("""
struct Cursor {
  int position;
  int read() const;
  friend bool operator==(const Cursor &, const Cursor &);
};
struct Unused { int discarded; };
""", "int inspect(const Cursor &c) { return c.read(); }")
        self.assertIn("friend bool operator==", text)
        self.assertNotIn("struct Unused", text)

    def test_explicit_instantiations_follow_concrete_type_dependencies(self):
        text, _ = self.slice("""
struct Needed { int value; };
struct Discarded { int value; };
template<typename T> struct Box { T value; };
template struct Box<Needed>;
template struct Box<Discarded>;
""", "int inspect(Box<Needed> b) { return b.value.value; }")
        self.assertIn("template struct Box<Needed>;", text)
        self.assertNotIn("template struct Box<Discarded>;", text)
        self.assertNotIn("struct Discarded", text)

    def test_explicit_instantiation_keeps_qualified_trait_owner(self):
        text, _ = self.slice("""
struct Traits { template<typename T> struct Trait {}; };
struct Needed;
struct Unused;
template<typename T, template<typename> class Trait> struct Box : Trait<T> {};
template struct Box<Needed, Traits::Trait>;
template struct Box<Unused, Traits::Trait>;
struct Needed : Box<Needed, Traits::Trait> { int read() const; };
""", "int inspect(const Needed &n) { return n.read(); }")
        self.assertIn("struct Traits", text)
        self.assertIn("template struct Box<Needed, Traits::Trait>", text)
        self.assertNotIn("template struct Box<Unused", text)

    def test_template_partial_specialization_and_alias(self):
        text, _ = self.slice("""
template <typename T> struct Holder {
  using Value = T;
  T value;
  Value read() const { return value; }
  int unused() const;
};
template <typename T> struct Holder<T *> {
  T *value;
  T read() const { return *value; }
  int unused() const;
};
using View = Holder<int *>;
""", "int inspect(const View &h) { return h.read(); }")
        self.assertIn("Holder<T *>", text)
        self.assertIn("using View", text)
        self.assertIn("return *value", text)
        self.assertNotIn("unused()", text)

    def test_unused_inherited_constructor_does_not_root_its_class(self):
        text, _ = self.slice("""
struct Base { Base(int); };
struct Unused : Base {
  using Base::Base;
  struct Nested;
  int unused() const;
};
struct Needed { int used() const; };
""", "int inspect(const Needed &n) { return n.used(); }")
        self.assertIn("struct Needed", text)
        self.assertNotIn("struct Unused", text)
        self.assertNotIn("struct Base", text)

    def test_virtual_and_special_member_surface(self):
        text, _ = self.slice("""
struct Base {
  Base() = default;
  Base(const Base &) = delete;
  virtual ~Base() = default;
  virtual int entry() const = 0;
  int unused() const;
};
""", "int inspect(const Base &b) { return b.entry(); }")
        self.assertIn("Base(const Base &) = delete", text)
        self.assertIn("virtual ~Base()", text)
        self.assertIn("virtual int entry()", text)
        self.assertNotIn("unused()", text)

    def test_dependent_base_alias_and_unresolved_call(self):
        text, _ = self.slice("""
template <bool> struct Flag {};
template <bool B> using Condition = Flag<B>;
int helper(int);
template <typename T> struct Node : Condition<(sizeof(T) > 0)> {
  T value;
  int used() const { return helper(value); }
  int unused() const;
};
""", "int inspect(const Node<int> &n) { return n.used(); }")
        self.assertIn("using Condition", text)
        self.assertIn("int helper(int)", text)
        self.assertNotIn("unused()", text)

    def test_constraint_keeps_dependent_container_member(self):
        text, _ = self.slice("""
struct Container { const int *data() const; int unused() const; };
template <typename T> struct View {
  template <typename C, typename = decltype(C().data())> View(const C &);
  int size() const;
};
""", "int inspect(const Container &c) { View<int> v(c); return v.size(); }")
        self.assertIn("const int *data()", text)
        self.assertIn("decltype(C().data())", text)
        self.assertNotIn("unused()", text)

    def test_dependent_using_preserves_target_member(self):
        text, _ = self.slice("""
template <typename T> struct Base {
  int forwarded() const;
  int unused() const;
};
template <typename T> struct Derived : Base<T> {
  using Base<T>::forwarded;
  int used() const { return 1; }
};
""", "int inspect(const Derived<int> &d) { return d.used(); }")
        self.assertIn("int forwarded()", text)
        self.assertNotIn("unused()", text)

    def test_dependent_static_lookup_in_return_type(self):
        text, _ = self.slice("""
template <typename T> struct CastInfo {
  static T convert(int);
  static int unused();
};
template <typename T> auto convert(int value) -> decltype(CastInfo<T>::convert(value)) {
  return CastInfo<T>::convert(value);
}
""", "int inspect() { return convert<int>(1); }")
        self.assertIn("static T convert(int)", text)
        self.assertNotIn("unused()", text)

    def test_stale_shared_library_plan_is_rejected(self):
        self.slice("struct Handle { int used() const; };",
                   "int inspect(const Handle &h) { return h.used(); }", mutate=True)

    def test_retained_function_keeps_whole_local_class_and_lambda(self):
        text, _ = self.slice("""
inline int entry() {
  struct Local { int used() const { return 1; } int notCalled() const { return 2; } };
  Local object;
  auto callback = [&]() { return object.used(); };
  return callback();
}
inline int unused() { return 3; }
""", "int inspect() { return entry(); }")
        self.assertIn("notCalled()", text)
        self.assertIn("return callback();", text)
        self.assertNotIn("int unused()", text)


if __name__ == "__main__":
    unittest.main()
