"""Branch normalization, ancestor ownership and fresh-AST convergence across profiles."""

import unittest

from rewrite_support import RewriteCase, rest

PROFILES = ("reshape", "matmul-scalar", "lower-ime", "silu-fusion",
            "assume-tight", "eliminate-copy", "gpu-memcpy", "depthwise-conv",
            "builtin-transpose")
HANDLE = r'''
int calls = 0;
struct Handle {
  int n;
  explicit operator bool() const { return n != 0; }
  bool operator!() const { return true; }
};
Handle query(int n) { ++calls; return {n}; }
'''


class IfConditionTests(RewriteCase):
    def test_unbraced_chain_normalizes_before_expansion_in_every_profile(self):
        code = HANDLE + r'''
int choose(int a, int b) {
  if (auto x = query(a)) return x.n;
  else if (auto y = query(b)) return y.n;
  else return 9;
}
int main() {
  calls=0; if (choose(3,4)!=3 || calls!=1) return 1;
  calls=0; if (choose(0,4)!=4 || calls!=2) return 2;
  calls=0; return !(choose(0,0)==9 && calls==2);
}
'''
        for profile in PROFILES:
            with self.subTest(profile=profile):
                output, report = self.rewrite_to_fixpoint(code, "if-condition", profile=profile)
                self.assertEqual([e["rule"] for e in report["edits"]],
                                 ["if-condition-normalize", "if-condition"])
                normalized = report["edits"][0]["after"]
                self.assertIn("else if (auto y", normalized)
                self.assertNotIn("static_cast<bool>", normalized)
                self.assertEqual(report["rounds"][-1]["edit_count"], 0)
                self.compare_behavior(output)

    def test_nested_unbraced_if_preserves_else_binding_and_query_timing(self):
        code = HANDLE + r'''
int choose(int a, int b) {
  int result = -1;
  if (auto x = query(a))
    if (auto y = query(b))
      result = x.n + y.n;
    else result = 10;
  else result = 20;
  return result;
}
int main() {
  calls=0; if (choose(2,3)!=5 || calls!=2) return 1;
  calls=0; if (choose(2,0)!=10 || calls!=2) return 2;
  calls=0; return !(choose(0,3)==20 && calls==1);
}
'''
        for profile in PROFILES:
            with self.subTest(profile=profile):
                output, report = self.rewrite_to_fixpoint(code, "if-condition", profile=profile)
                self.assertEqual([e["rule"] for e in report["edits"]],
                                 ["if-condition-normalize", "if-condition",
                                  "if-condition-normalize", "if-condition"])
                self.assertNotIn("if (auto", output.read_text())
                self.compare_behavior(output)

    def test_separate_roots_converge_and_keep_multiline_literal_bytes(self):
        code = r'''
int run() {
  int result = 0;
  const char *text = nullptr;
  if (auto x = 1) {
    result += x;
  }
  if (auto y = 1)
    text = R"tag(first
  second
)tag";
  return result == 1 && text[6]==' ' && text[7]==' ' && text[8]=='s';
}
int main() { return !run(); }
'''
        output, report = self.rewrite_to_fixpoint(code, "if-condition", profile="silu-fusion")
        self.assertEqual(sum(e["rule"] == "if-condition" for e in report["edits"]), 2)
        self.assertIn('R"tag(first\n  second\n)tag"', output.read_text())
        self.compare_behavior(output)
        final = output.read_text()
        _, second = self.rewrite_to_fixpoint(final, "if-condition", profile="silu-fusion")
        self.assertFalse(second["edits"])

    def test_cleanup_policy_is_uniform_and_rejects_before_normalizing(self):
        invalid = [
            'struct H { ~H() {} operator bool() const; }; void f(){if(H h=H{}) return;}',
            'struct H { operator bool() const; }; struct T { ~T(); H get(); }; '
            'void f(){if(H h=T().get()) return;}',
            'void f(){if(volatile int x=1) return;}',
            '#define BODY return\nvoid f(){if(auto x=1) BODY;}',
            'void f(){if(auto x=1) {goto done;} done:;}',
        ]
        for profile in PROFILES:
            for code in invalid:
                with self.subTest(profile=profile, code=code):
                    (self.root / "report.json").unlink(missing_ok=True)
                    _, report = self.rewrite(code, "if-condition", expected=1, profile=profile)
                    self.assertTrue(report["rejections"])

    def test_legacy_include_substitution_runs_after_convergence(self):
        from experiment.artifacts import write_baseline_and_patch

        # A declaration before the include deliberately shifts its final-round
        # byte offset; original_offset must still replay the baseline correctly.
        header = self.root / "mlir/IR/Builders.h"
        header.parent.mkdir(parents=True)
        header.write_text("// real include fixture\n")
        (self.root / "mini_mlir.h").write_text("// legacy include fixture\n")
        code = 'int choose(){if(auto x=1)return x;else return 0;}\n' \
               '#include "mlir/IR/Builders.h"\nint main(){return choose()!=1;}\n'
        output, report = self.rewrite_to_fixpoint(code, "if-condition", profile="silu-fusion",
                                                  shared_mini=False)
        includes = [e for e in report["edits"] if e["rule"] == "interface-substitution"]
        self.assertEqual(len(includes), 1)
        self.assertNotEqual(includes[0]["offset"], includes[0]["original_offset"])
        self.assertEqual(report["rounds"][-1]["rule"], "interface-substitution")
        self.compare_behavior(output)
        from types import SimpleNamespace
        write_baseline_and_patch(SimpleNamespace(source=self.root / "source.cpp", out=self.root), report)
        baseline = (self.root / "baseline_mini.cpp").read_text()
        self.assertIn("if(auto x=1)return x;else return 0;", baseline)
        self.assertIn('#include "mini_mlir.h"', baseline)


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
