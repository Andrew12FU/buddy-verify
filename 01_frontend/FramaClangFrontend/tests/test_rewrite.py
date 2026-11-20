#!/usr/bin/env python3
"""小程序行为对照与拒绝路径。完整 pass 的三组原生检查和 Frama 转换由 run.py 承担。"""

import json
from pathlib import Path
import subprocess
import unittest

from rewrite_support import RewriteCase, args, rest


class RewriteTests(RewriteCase):
    def test_while_evaluation_continue_shadowing_and_bool(self):
        """核对每轮求值、continue、同名遮蔽，以及 bool 转换不会误选重载否定。"""
        output, report = self.rewrite("""
struct Handle {
  int n;
  explicit operator bool() const { return n < 4; }
  bool operator!() const { return true; } // Must not be selected by the rewrite.
};
int main() {
  int calls = 0, sum = 0;
  while (auto h = Handle{++calls}) {
    if (h.n == 2) continue;
    { int h = 10; sum += h; }
  }
  return !(calls == 4 && sum == 20);
}
""", "while")
        self.assertEqual(len(report["edits"]), 1)
        self.compare_behavior(output)

    def test_capture_original_identity_shadowing_and_collision(self):
        """核对原对象被修改，同名局部对象不被误改，生成名字避开冲突。"""
        output, report = self.rewrite("""
struct Box { int value; void add(int n) { value += n; } };
template<class F> void now(F f) { f(); }
int main() {
  Box box{1}, other{2};
  int boxPtr = 4;
  now([&]() { box.add(other.value); { Box box{99}; box.add(1); } });
  return !(box.value == 3 && other.value == 2 && boxPtr == 4);
}
""", "capture")
        self.assertIn("boxPtr1", output.read_text())
        self.assertEqual(len(report["edits"]), 2)
        self.compare_behavior(output)

    def test_namespace_reopening_and_external_type_reference(self):
        """同一匿名空间重开、类外定义和外部类型引用使用一致的新名字。"""
        output, report = self.rewrite("""
namespace { struct A { int get(); }; }
namespace { int value = 7; }
int A::get() { return value; }
template<class T> int use() { T a; return a.get(); }
int main() { return use<A>() != 7; }
""", "namespace")
        self.assertEqual(sum(e["rule"] == "namespace" for e in report["edits"]), 2)
        self.compare_behavior(output)

    def test_guard_short_circuit_and_idempotence(self):
        """检查 Reshape guard 的求值次数、返回结果及第二次改写的幂等性。"""
        output, report = self.rewrite("""
namespace mlir {
int calls = 0;
struct RankedTensorType { bool ok; explicit operator bool() const { ++calls; return ok; } };
int failure() { return 3; }
int check(RankedTensorType inTy, RankedTensorType outTy) {
  if (!inTy || !outTy) return failure();
  return 0;
}
}
int main() {
  int a = mlir::check({false}, {true});
  int b = mlir::check({true}, {true});
  return !(a == 3 && b == 0 && mlir::calls == 3);
}
""", "guard")
        self.assertEqual(len(report["edits"]), 1)
        self.compare_behavior(output)
        second = self.root / "second.cpp"
        second_report = self.root / "second.json"
        result = subprocess.run([args.tool, output, "--output", second, "--report",
                                 second_report, "--rules=guard", "--", "-std=c++17"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.read_bytes(), second.read_bytes())
        self.assertFalse(json.loads(second_report.read_text())["edits"])

    def test_reject_nontrivial_condition_lifetime(self):
        """条件对象非平凡析构的生命周期超出本轮能力，必须拒绝。"""
        _, report = self.rewrite("""
struct X { ~X() {} operator bool() const { return false; } };
void f() { while (X x = X{}) {} }
""", "while", 1)
        self.assertTrue(report["rejections"])

    def test_reject_overloaded_address(self):
        """成员 operator& 可能改变取地址含义，必须拒绝。"""
        self.rewrite("""
struct X { X *operator&(); void f(); };
template<class F> void now(F f) { f(); }
void f() { X x; now([&]() { x.f(); }); }
""", "capture", 1)

    def test_reject_free_address_operator(self):
        """自由一元 operator& 可能经 ADL 被选中，同样拒绝。"""
        self.rewrite("""
namespace n {
struct X { void f(); };
X *operator&(X &);
template<class F> void now(F f) { f(); }
void f() { X x; now([&]() { x.f(); }); }
}
""", "capture", 1)

    def test_reject_initializer_temporary_cleanup(self):
        """初始化临时对象的清理可能影响条件，必须拒绝。"""
        self.rewrite("""
struct Flag { operator bool() const; };
struct Maker { ~Maker(); Flag get(); };
void f() { while (Flag flag = Maker().get()) {} }
""", "while", 1)

    def test_namespace_macro_inside_is_preserved(self):
        """匿名空间内部的宏类型引用保留，不编辑展开 token。"""
        output, report = self.rewrite("""
#define DECLARE_SELF(T) static T *self();
namespace { struct A { DECLARE_SELF(A) }; }
int main() { A a; (void)a; }
""", "namespace")
        self.assertIn("DECLARE_SELF(A)", output.read_text())
        self.compare_behavior(output)

    def test_guard_overloaded_not_and_llvm_failure_alias(self):
        """识别真实接口的重载 ! 和 LLVM failure 别名。"""
        output, report = self.rewrite("""
namespace llvm { int failure() { return 3; } }
namespace mlir {
using llvm::failure;
struct RankedTensorType { bool ok; bool operator!() const { return !ok; } };
int check(RankedTensorType a, RankedTensorType b) {
  if (!a || !b) return failure();
  return 0;
}
}
int main() { return !(mlir::check({true}, {false}) == 3); }
""", "guard")
        self.assertEqual(len(report["edits"]), 1)
        self.compare_behavior(output)

    def test_plain_bool_or_is_rewritten(self):
        """普通布尔条件和任意返回值使用同一 AST 规则。"""
        code = "int f(bool a, bool b) { if (!a || !b) return 3; return 0; }"
        output, report = self.rewrite(code, "guard")
        self.assertEqual(len(report["edits"]), 1)
        self.assertIn("bool guardFailed = (!a || !b);", output.read_text())
        self.assertIn("return 3;", output.read_text())

    def test_guard_short_circuit_in_methods_loops_and_main(self):
        """覆盖全部短路叶子，统一处理循环和 main 内的条件，并检查幂等性。"""
        code = """
namespace llvm { int failure() { return 1; } }
namespace mlir {
using llvm::failure;
int calls = 0;
bool wrongOrder = false;
bool guardFailed = false; // Generated guard locals must avoid this identifier.
bool test(int n, int failing) {
  wrongOrder |= ++calls != n;
  return n == failing;
}
struct ReplaceScalarLikeMatmul {
  int matchAndRewrite(int failing) const {
    if (test(1, failing) || test(2, failing) || test(3, failing)) return failure();
    if (test(4, failing) || test(5, failing) || test(6, failing)) return failure();
    if (test(7, failing) || test(8, failing)) return failure();
    if (test(9, failing) || test(10, failing) || test(11, failing) ||
        test(12, failing) || test(13, failing) || test(14, failing)) return failure();
    if (test(15, failing) || test(16, failing) || test(17, failing)) return failure();
    if (test(18, failing) || test(19, failing)) return failure();
    for (int i = 0; i < 1; ++i) {
      if (test(20, failing) || test(21, failing) || test(22, failing)) return failure();
    }
    return 0;
  }
};
}
int main() {
  mlir::ReplaceScalarLikeMatmul pattern;
  for (int f = 0; f <= 22; ++f) {
    mlir::calls = 0;
    int result = pattern.matchAndRewrite(f);
    if (result != (f != 0) || mlir::calls != (f ? f : 22) || mlir::wrongOrder) return 1;
  }
}
"""
        output, report = self.rewrite(code, "guard", profile="matmul-scalar")
        self.assertEqual(len(report["edits"]), 8)
        self.assertEqual(report["profile"], "matmul-scalar")
        self.assertIn("bool guardFailed1 = (", output.read_text())
        self.assertIn("bool guardFailed8 = (", output.read_text())
        self.assertTrue(all("bool guardFailed" in e["after"] for e in report["edits"]))
        loop = "if (test(20, failing) || test(21, failing) || test(22, failing)) return failure();"
        self.assertNotIn(loop, output.read_text())
        self.assertTrue(any("test(20" in e["before"] for e in report["edits"]))
        self.compare_behavior(output)
        second, second_report = self.root / "second.cpp", self.root / "second.json"
        result = subprocess.run([args.tool, output, "--output", second, "--report",
                                 second_report, "--profile=matmul-scalar", "--rules=guard",
                                 "--", "-std=c++17"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.read_bytes(), second.read_bytes())
        self.assertFalse(json.loads(second_report.read_text())["edits"])

    def test_guard_accepts_all_methods_else_init_and_returns(self):
        """匹配不依赖方法、类、else 或返回值，仍排除重载逻辑或。"""
        code = """
namespace mlir {
int failure() { return 1; }
struct Flag {};
bool operator||(Flag, Flag) { return false; }
struct ReplaceScalarLikeMatmul {
  int runOnOperation(bool a, bool b) {
    if (a || b) return failure();
    return 0;
  }
  int matchAndRewrite(Flag a, Flag b) {
    if (a || b) return failure();
    return 0;
  }
  int matchAndRewrite(bool a, bool b) {
    if (bool c = a; c || b) return failure();
    if (a || b) return 7;
    if (a || b) return failure(); else return 0;
  }
};
struct OtherPattern {
  int matchAndRewrite(bool a, bool b) {
    if (a || b) return failure();
    return 0;
  }
};
}
"""
        output, report = self.rewrite(code, "guard", profile="matmul-scalar")
        self.assertEqual(len(report["edits"]), 5)
        self.assertIn("else return 0;", output.read_text())
        self.assertIn("return 7;", output.read_text())
        self.assertIn("if (a || b) return failure();", output.read_text())

    def test_guard_in_free_function_is_profile_independent(self):
        """自由函数中的同类 AST 结构不再根据 profile 排除。"""
        code = """
namespace mlir {
struct RankedTensorType { explicit operator bool() const; };
int failure();
int check(RankedTensorType a, RankedTensorType b) {
  if (!a || !b) return failure();
  return 0;
}
}
"""
        output, report = self.rewrite(code, "guard", profile="matmul-scalar")
        self.assertEqual(len(report["edits"]), 1)
        self.assertIn("bool guardFailed = (!a || !b);", output.read_text())

    def test_matmul_include_allowlist_preserves_unrelated_header(self):
        """只替换八个白名单 include，保留无关头文件。"""
        names = ["mlir/Dialect/Func/IR/FuncOps.h", "mlir/Dialect/Tosa/IR/TosaOps.h",
                 "mlir/IR/Builders.h", "mlir/IR/BuiltinAttributes.h",
                 "mlir/IR/BuiltinTypes.h", "mlir/IR/PatternMatch.h",
                 "mlir/Pass/Pass.h", "mlir/Transforms/GreedyPatternRewriteDriver.h",
                 "keep.h"]
        for name in names:
            header = self.root / name
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text("// stub header for include selection\n")
        code = "".join(f'#include "{name}"\n' for name in names) + "int main() {}\n"
        output, report = self.rewrite(code, "namespace", profile="matmul-scalar",
                                      options=["--interface-header=mini_mlir.h"])
        self.assertEqual(len(report["edits"]), 8)
        self.assertEqual(output.read_text(), '#include "mini_mlir.h"\n#include "keep.h"\nint main() {}\n')

    def test_reject_macro_capture_use(self):
        """对象通过宏使用时原 token 不可安全编辑，必须拒绝。"""
        self.rewrite("""
#define USE(x) x.f()
struct X { void f(); };
template<class F> void now(F f) { f(); }
void f() { X x; now([&]() { USE(x); }); }
""", "capture", 1)

    def test_reject_nested_copy_capture(self):
        """按值复制捕获建立新的对象身份，不能沿用引用捕获的指针传播。"""
        self.rewrite("""
struct X { void f(); };
template<class F> void now(F f) { f(); }
void f() { X x; now([&]() { now([x]() mutable { x.f(); }); }); }
""", "capture", 1)

    def test_reject_overlapping_rules(self):
        """同一轮 while 与捕获改写范围重叠时整份计划拒绝。"""
        self.rewrite("""
struct X { operator bool() const; void f(); };
template<class F> void now(F f) { f(); }
void f() { while (X x = X{}) { now([&]() { x.f(); }); } }
""", "while,capture", 1)

    def test_parse_error_never_emits_output(self):
        """无效 C++ 不得产生伪成功源码。"""
        self.rewrite("void f() { missing_type x; }", "while", 1)

    def test_protect_input_even_through_symlink(self):
        """输出经符号链接指向原件也不得覆盖。"""
        source = self.root / "source.cpp"
        source.write_text("int main() {}")
        link = self.root / "link.cpp"
        link.symlink_to(source)
        result = subprocess.run([args.tool, source, "--output", link, "--report",
                                 self.root / "report.json", "--", "-std=c++17"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertEqual(source.read_text(), "int main() {}")


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
