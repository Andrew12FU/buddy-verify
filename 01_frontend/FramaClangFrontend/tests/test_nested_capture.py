#!/usr/bin/env python3
"""嵌套捕获的指针传播、局部作用域、组合补丁、幂等性和拒绝路径。"""

import json
from pathlib import Path
import subprocess
import unittest

from rewrite_support import FLAGS, RewriteCase, args, rest

FIXTURES = Path(__file__).resolve().parent / "fixtures/nested_capture"


class NestedCaptureTests(RewriteCase):
    def second_round(self, output):
        """对嵌套组重复改写，要求零编辑和相同源码，防止处理生成的指针值捕获。"""
        second, report = self.root / "second.cpp", self.root / "second.json"
        result = subprocess.run([args.tool, output, "--output", second,
                                 "--report", report, "--rules=capture", "--", *FLAGS],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(second.read_bytes(), output.read_bytes())
        self.assertFalse(json.loads(report.read_text())["edits"])

    def check_fixture(self, name, pointers):
        """编译执行独立源样例前后版本，检查整组单替换和幂等性。"""
        output, report = self.rewrite((FIXTURES / name).read_text(), "capture")
        self.assertEqual(sum(e["rule"] == "capture" for e in report["edits"]), 1)
        self.assertEqual(sum(e["rule"] == "capture-pointers" for e in report["edits"]), pointers)
        self.assertIn("nested lambdas", next(e["reason"] for e in report["edits"]
                                              if e["rule"] == "capture"))
        self.compare_behavior(output)
        self.second_round(output)
        return output

    def test_two_levels_modify_original(self):
        """两层闭包都显式按值捕获同一原对象指针，不为内层再取地址。"""
        output = self.check_fixture("two_levels.cpp", 1)
        self.assertEqual(output.read_text().count("[boxPtr]"), 1)
        self.assertEqual(output.read_text().count("[boxPtr1]"), 1)
        self.assertIn("auto *boxPtr1 = boxPtr;", output.read_text())
        self.assertEqual(output.read_text().count("auto *boxPtr = &box;"), 1)

    def test_local_object_address_stays_inside_outer_body(self):
        """外层局部对象的指针在其声明之后、内层初始化之前创建。"""
        output = self.check_fixture("local_object.cpp", 1)
        text = output.read_text()
        self.assertLess(text.index("Box local{3};"), text.index("auto *localPtr = &local;"))
        self.assertLess(text.index("auto *localPtr = &local;"), text.index("auto inner ="))

    def test_returned_three_levels(self):
        """外层临时闭包已销毁后调用其返回的内层闭包，原对象仍存活。"""
        output = self.check_fixture("returned_three_levels.cpp", 1)
        self.assertEqual(output.read_text().count("[boxPtr]"), 1)
        self.assertIn("[boxPtr1]", output.read_text())
        self.assertIn("[boxPtr2]", output.read_text())
        self.assertIn("auto *boxPtr2 = boxPtr1;", output.read_text())
        self.assertNotIn("&boxPtr", output.read_text())

    def test_empty_outer_with_local_capture(self):
        """外层无捕获也要接管有类引用捕获的子闭包，根前无需指针声明。"""
        output, report = self.rewrite(r'''
struct Box { int n; };
int main() {
  auto outer = []() {
    Box local{2};
    auto inner = [&]() { local.n += 3; };
    inner();
    return local.n;
  };
  return outer() != 5;
}
''', "capture")
        self.assertEqual(len(report["edits"]), 1)
        self.assertEqual(report["edits"][0]["rule"], "capture")
        self.compare_behavior(output)
        self.second_round(output)

    def test_siblings_share_statement_and_preserve_shadowing(self):
        """兄弟闭包共享前置声明位置；同名对象按声明身份映射，生成名避让。"""
        output, report = self.rewrite(r'''
struct Box { int n; };
template<class F, class G> void both(F f, G g) { f(); g(); }
int main() {
  Box result{0};
  int localPtr = 4;
  auto outer = [&]() {
    Box local{2};
    both([&]() { result.n += local.n; }, [&]() {
      result.n += local.n;
      { Box local{99}; result.n += local.n; }
    });
  };
  outer();
  return !(result.n == 103 && localPtr == 4);
}
''', "capture")
        text = output.read_text()
        self.assertIn("localPtr1", text)
        self.assertIn("localPtr2", text)
        self.assertIn("Box local{99}; (*resultPtr2).n += local.n;", text)
        self.assertEqual(sum(e["rule"] == "capture" for e in report["edits"]), 1)
        self.compare_behavior(output)
        self.second_round(output)

    def test_parent_parameter_captured_by_inner(self):
        """外层 lambda 参数的地址在该次调用体内获取，不提到外层闭包之前。"""
        output, _ = self.rewrite(r'''
struct Box { int n; };
int main() {
  auto outer = [](Box parameter) {
    auto inner = [&]() { parameter.n += 2; };
    inner();
    return parameter.n;
  };
  return outer({3}) != 5;
}
''', "capture")
        self.compare_behavior(output)
        self.second_round(output)

    def test_immediately_invoked_root_and_child(self):
        """根语句和内层语句从 lambda 起始时，前奏与 introducer 替换可显式组合。"""
        output, report = self.rewrite(r'''
struct Box { int n; };
int main() {
  Box box{1};
  [&]() {
    Box local{4};
    [&]() { box.n += local.n; }();
  }();
  return box.n != 5;
}
''', "capture")
        self.assertEqual(len(report["edits"]), 1)
        self.assertIn("auto *boxPtr = &box;", output.read_text())
        self.assertIn("auto *localPtr = &local;", output.read_text())
        self.compare_behavior(output)
        self.second_round(output)

    def test_scalar_nested_capture_is_rewritten(self):
        """标量引用也属于候选，并逐层传播指针。"""
        code = 'int main() { int n=1; auto f=[&](){ auto g=[&](){return ++n;}; return g();}; return !(f()==2 && n==2); }'
        output, report = self.rewrite(code, "capture")
        self.assertTrue(report["edits"])
        self.compare_behavior(output)
        self.second_round(output)

    def test_copy_only_tree_is_unchanged(self):
        code = 'int main() { int n=1; auto f=[n](){ auto g=[n](){return n;}; return g();}; return f()!=1; }'
        output, report = self.rewrite(code, "capture")
        self.assertFalse(report["edits"])
        self.assertEqual(output.read_text(), code)
        self.compare_behavior(output)

    def test_reference_to_parent_copy_keeps_copy_identity(self):
        """内层对外层副本取地址，不能指向函数中的原对象。"""
        output, _ = self.rewrite(r'''
struct Box { int n; };
int main() {
  Box box{1};
  auto outer = [box]() mutable {
    auto inner = [&box]() { box.n += 2; };
    inner(); return box.n;
  };
  box.n = 10;
  return !(outer() == 3 && box.n == 10);
}
''', "capture")
        self.compare_behavior(output)
        self.second_round(output)

    def test_mixed_scalar_and_record_references(self):
        """类对象和标量引用使用相同地址传播机制。"""
        output, _ = self.rewrite(r'''
struct Box { int n; };
int main() {
  Box box{1}; int n=2;
  auto outer = [&]() {
    auto inner = [&]() { box.n += n; ++n; };
    inner();
  };
  outer();
  return !(box.n == 3 && n == 3);
}
''', "capture")
        self.compare_behavior(output)
        self.second_round(output)

    def test_reject_nested_init_capture(self):
        """初始化捕获建立新对象身份，当前不能当作同一个原对象传播。"""
        self.rewrite(r'''
struct Box { int n; };
template<class F> void now(F f) { f(); }
void f() { Box box{1}; now([&]() { now([&copy=box]() { copy.n++; }); }); }
''', "capture", 1)

    def test_reject_generic_child(self):
        """子闭包模板实例化可能重复源码节点，当前整组拒绝。"""
        self.rewrite(r'''
struct Box { int n; };
template<class F> void now(F f) { f(); }
void f() { Box box{1}; now([&]() { auto g=[&](auto){box.n++;}; g(1); }); }
''', "capture", 1)

    def test_reject_macro_child_use(self):
        """内层的宏引用不能靠父层的源码位置匹配绕过宏拒绝检查。"""
        self.rewrite(r'''
#define USE(x) x.n++
struct Box { int n; };
template<class F> void now(F f) { f(); }
void f() { Box box{1}; now([&]() { now([&]() { USE(box); }); }); }
''', "capture", 1)

    def test_reject_child_unevaluated_use(self):
        """sizeof 等节点中的引用不能被盲目替换，子层问题应拒绝整个根计划。"""
        self.rewrite(r'''
struct Box { int n; };
template<class F> void now(F f) { f(); }
void f() { Box box{1}; now([&]() { now([&]() { box.n += sizeof(box); }); }); }
''', "capture", 1)

    def test_reject_macro_generated_child(self):
        """先验证所有子闭包的源码范围，宏生成的整个 lambda 不能用于偏移运算。"""
        self.rewrite(r'''
#define CHILD [&](){box.n++;}
struct Box { int n; };
template<class F> void now(F f) { f(); }
void f() { Box box{1}; now([&]() { now(CHILD); }); }
''', "capture", 1)

    def test_reject_child_noexcept_use(self):
        """noexcept 也是未求值上下文，不能遗漏该拒绝条件。"""
        self.rewrite(r'''
struct Box { int n; void add() noexcept {n++;} };
template<class F> void now(F f) { f(); }
void f() { Box box{1}; now([&]() { now([&]() { box.n += noexcept(box.add()); }); }); }
''', "capture", 1)

    def test_reject_local_pointer_in_conditional(self):
        """外层局部对象只在条件路径中被内层捕获时，不把取地址提前跨过该条件。"""
        self.rewrite(r'''
struct Box { int n; };
template<class F> bool call(F f) { f(); return true; }
template<class F> void now(F f) { f(); }
void f() { now([]() { Box local{1}; bool n=true && call([&](){local.n++;}); }); }
''', "capture", 1)


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
