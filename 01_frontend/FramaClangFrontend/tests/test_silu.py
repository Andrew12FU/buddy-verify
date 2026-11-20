#!/usr/bin/env python3
"""SiLU 新规则的行为、协作及拒绝路径；LowerIME 的独立 include 范围。"""

import json
import subprocess
import unittest

from rewrite_support import RewriteCase, args, rest, FLAGS

# 模型只用于运行规则行为对照；完整 pass 回归使用历史已审计 mini。
MODEL = r'''
#include <initializer_list>
#include <cstddef>
namespace llvm {
template<class T> struct ArrayRef {
  const T *p; std::size_t n;
  ArrayRef(const T *p, std::size_t n): p(p), n(n) {}
};
}
namespace mlir {
struct Value { int n; };
struct ValueRange {
  const Value *p; std::size_t n;
  ValueRange(std::initializer_list<Value> v): p(v.begin()), n(v.size()) {}
  ValueRange(llvm::ArrayRef<Value> v): p(v.p), n(v.n) {}
};
namespace linalg {
struct GenericOp {
  int sum;
  template<class F> static GenericOp create(int &b, int &loc, int &type,
      ValueRange input, ValueRange output, int &maps, int &iters, F f) {
    int sum = b + loc + type + maps + iters;
    for (std::size_t i=0; i<input.n; ++i) sum += input.p[i].n;
    for (std::size_t i=0; i<output.n; ++i) sum += output.p[i].n;
    f();
    return {sum};
  }
};
}
}
'''


class SiLURewriteTests(RewriteCase):
    def second_round(self, output, rules):
        """再次执行相同规则，要求无编辑且字节相同；返回值不代替 Frama 检查。"""
        second, report = self.root / "second.cpp", self.root / "second.json"
        result = subprocess.run([args.tool, output, "--output", second,
                                 "--report", report, "--profile=silu-fusion",
                                 f"--rules={rules}", "--", *FLAGS],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.read_bytes(), second.read_bytes())
        self.assertFalse(json.loads(report.read_text())["edits"])

    def test_condition_chain_queries_priority_and_shadowing(self):
        """覆盖左匹配、右匹配、都不匹配及左匹配但内条件失败；不提前查询右侧。"""
        output, report = self.rewrite(r'''
int calls;
struct Handle {
  int n;
  explicit operator bool() const { return n != 0; }
  bool operator!() const { return true; }
};
Handle query(int n) { ++calls; return {n}; }
int run(int a, int b) {
  int result = 0;
  if (auto lhs = query(a)) {
    if (lhs.n == 1) { int lhs = 7; result = lhs; }
  } else if (auto rhs = query(b)) {
    result = rhs.n;
  }
  return result;
}
int main() {
  calls=0; if (run(1,2)!=7 || calls!=1) return 1;
  calls=0; if (run(3,2)!=0 || calls!=1) return 2;
  calls=0; if (run(0,2)!=2 || calls!=2) return 3;
  calls=0; if (run(0,0)!=0 || calls!=2) return 4;
}
''', "if-condition", profile="silu-fusion")
        self.assertEqual(len(report["edits"]), 1)
        self.compare_behavior(output)
        self.second_round(output, "if-condition")

    def test_condition_plain_else_and_multiline_literal(self):
        """普通 else 的分号完整保留；嵌套缩进不能修改原始多行字符串。"""
        output, report = self.rewrite_to_fixpoint(r'''
int run(bool flag) {
  if (auto x = flag) {
    const char *text = R"tag(first
  second
)tag";
    return text[6] == ' ' && text[7] == ' ' && text[8] == 's' ? 7 : 1;
  } else return 9;
}
int main() { return !(run(true) == 7 && run(false) == 9); }
''', "if-condition", profile="silu-fusion")
        self.assertEqual([e["rule"] for e in report["edits"]],
                         ["if-condition-normalize", "if-condition"])
        self.compare_behavior(output)

    def test_other_profiles_accept_declaration_capture(self):
        """单个自动局部声明按结构接受，不再要求 SiLU profile。"""
        self.rewrite('struct Box { int n; }; template<class F> int call(F f) { return f(); } '
                     'void f() { Box b{1}; int n=call([&](){return b.n;}); }',
                     "capture", profile="reshape")

    def test_reject_condition_destructor(self):
        """超出已审计的平凡析构条件对象必须拒绝，而不是移动其析构时机。"""
        self.rewrite('struct X { ~X() {} operator bool() const; }; '
                     'void f() { if (X x = X{}) {} }',
                     "if-condition", 1, profile="silu-fusion")

    def test_reject_condition_macro(self):
        """宏产生的条件声明不尝试改写。"""
        self.rewrite('#define C auto x = 1\nvoid f() { if (C) {} }',
                     "if-condition", 1, profile="silu-fusion")

    def test_silu_guard_short_circuit_and_scope(self):
        """带块分支保留短路次数；循环、其他方法和 main 使用同一规则。"""
        output, report = self.rewrite(r'''
namespace mlir { int failure() { return 3; } }
int calls;
bool check(bool x) { ++calls; return x; }
struct SiLUFusionPattern {
 int matchAndRewrite(bool a, bool b) {
   if (check(a) || check(b)) { return mlir::failure(); }
   for (int i=0; i<1; ++i) { if (a || b) return 2; }
   return 0;
 }
 int other(bool a, bool b) { if (a || b) return mlir::failure(); return 0; }
};
int main() {
 SiLUFusionPattern p;
 calls=0; if(p.matchAndRewrite(true,false)!=3 || calls!=1) return 1;
 calls=0; if(p.matchAndRewrite(false,false)!=0 || calls!=2) return 2;
}
''', "guard", profile="silu-fusion")
        self.assertEqual(len(report["edits"]), 5)
        self.compare_behavior(output)
        self.second_round(output, "guard")

    def test_multivalue_range_and_capture_cooperate(self):
        """两个输入三个输出的元素顺序、原对象捕获和名字避让；显式同位置插入协作。"""
        output, report = self.rewrite(MODEL + r'''
struct Box { int n; };
int main() {
  int b=0, loc=0, type=0, maps=0, iters=0, rangeValues=9;
  mlir::Value a{1}, c{2}, d{3}, e{4}, g{5};
  Box box{7};
  auto op = mlir::linalg::GenericOp::create(b, loc, type,
      mlir::ValueRange{a, c}, mlir::ValueRange{d, e, g}, maps, iters,
      [&]() { box.n += 10; });
  return !(op.sum == 15 && box.n == 17 && rangeValues == 9);
}
''', "value-range,capture", profile="silu-fusion")
        rules = [e["rule"] for e in report["edits"]]
        self.assertEqual(rules.count("value-range"), 2)
        self.assertEqual(rules.count("value-range-preamble"), 1)
        self.assertEqual(rules.count("capture"), 1)
        self.assertEqual(rules.count("capture-pointers"), 1)
        inserts = [e for e in report["edits"] if not e["length"]]
        self.assertEqual(inserts[0]["offset"], inserts[1]["offset"])
        self.assertEqual([e["insertion_order"] for e in inserts], [10, 20])
        self.assertIn("rangeValues1", output.read_text())
        self.compare_behavior(output)
        self.second_round(output, "value-range,capture")

    def test_reject_range_argument_evaluation(self):
        """实参有调用时不能提前列表元素求值，必须拒绝。"""
        self.rewrite(MODEL + r'''
int &next();
void f() {
 int b=0, loc=0, type=0, maps=0, iters=0; mlir::Value a{1};
 auto op = mlir::linalg::GenericOp::create(next(), loc, type,
     mlir::ValueRange{a}, mlir::ValueRange{a}, maps, iters, [](){});
}
''', "value-range", 1, profile="silu-fusion")

    def test_reject_empty_range(self):
        """空列表不能生成标准 C++ 不允许的零长数组。"""
        self.rewrite(MODEL + r'''
void f() {
 int b=0, loc=0, type=0, maps=0, iters=0; mlir::Value a{1};
 auto op = mlir::linalg::GenericOp::create(b, loc, type,
     mlir::ValueRange{}, mlir::ValueRange{a}, maps, iters, [](){});
}
''', "value-range", 1, profile="silu-fusion")

    def test_reject_range_expression_element(self):
        """列表中的有副作用表达式不当作本地 Value 引用。"""
        self.rewrite(MODEL + r'''
mlir::Value next();
void f() {
 int b=0, loc=0, type=0, maps=0, iters=0; mlir::Value a{1};
 auto op = mlir::linalg::GenericOp::create(b, loc, type,
     mlir::ValueRange{next()}, mlir::ValueRange{a}, maps, iters, [](){});
}
''', "value-range", 1, profile="silu-fusion")

    def test_reject_callback_init_capture(self):
        """回调的显式捕获初始化可能有副作用，不改变它与实参的求值先后。"""
        self.rewrite(MODEL + r'''
int next();
void f() {
 int b=0, loc=0, type=0, maps=0, iters=0; mlir::Value a{1};
 auto op = mlir::linalg::GenericOp::create(b, loc, type,
     mlir::ValueRange{a}, mlir::ValueRange{a}, maps, iters, [n=next()](){});
}
''', "value-range", 1, profile="silu-fusion")

    def test_reject_static_capture_initializer(self):
        """静态初始化的闭包捕获时机不属于自动局部声明扩展。"""
        self.rewrite('struct Box { int n; }; template<class F> int call(F f) { return f(); } '
                     'void f() { Box b{1}; static int n=call([&](){return b.n;}); }',
                     "capture", 1, profile="silu-fusion")

    def test_explicit_rule_enablement_is_profile_independent(self):
        """显式启用后按 AST 匹配；默认启用集合仍由 runner 配置。"""
        code = 'void f() { if (auto x=1) {} }'
        output, report = self.rewrite(code, "if-condition,value-range", profile="reshape")
        self.assertEqual(sum(e["rule"] == "if-condition" for e in report["edits"]), 1)
        self.assertIn("static_cast<bool>(x)", output.read_text())

    def test_lower_ime_include_allowlist(self):
        """LowerIME 仅替换自己的十个 include，保留 TOSA 和不相关头。"""
        names = ["mlir/Conversion/LLVMCommon/ConversionTarget.h",
                 "mlir/Conversion/LLVMCommon/TypeConverter.h",
                 "mlir/Dialect/Arith/IR/Arith.h", "mlir/Dialect/Func/IR/FuncOps.h",
                 "mlir/Dialect/LLVMIR/LLVMDialect.h", "mlir/Dialect/MemRef/IR/MemRef.h",
                 "mlir/Pass/Pass.h", "Dialect/IME/IMEDialect.h", "Dialect/IME/IMEOps.h",
                 "Dialect/IME/Transform.h", "mlir/Dialect/Tosa/IR/TosaOps.h", "keep.h"]
        for name in names:
            header = self.root / name
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text("// include selection fixture\n")
        code = ''.join(f'#include "{name}"\n' for name in names) + 'int main() {}\n'
        output, report = self.rewrite(code, "namespace", profile="lower-ime",
                                      options=["--interface-header=mini_mlir.h"])
        self.assertEqual(len(report["edits"]), 10)
        self.assertEqual(output.read_text(), '#include "mini_mlir.h"\n'
                         '#include "mlir/Dialect/Tosa/IR/TosaOps.h"\n#include "keep.h"\nint main() {}\n')


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
