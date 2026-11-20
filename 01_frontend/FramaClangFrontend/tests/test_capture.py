#!/usr/bin/env python3
"""Generic capture: cross-profile equality, alias identity and value snapshots."""
import json
import subprocess
import unittest

from rewrite_support import FLAGS, RewriteCase, args, rest
from experiment.profiles import PASS_PROFILES


class CaptureTests(RewriteCase):
    def clear_outputs(self):
        for name in ("output.cpp", "report.json", "second.cpp", "second.json"):
            (self.root / name).unlink(missing_ok=True)

    def second_round(self, output, profile="reshape"):
        second, report = self.root / "second.cpp", self.root / "second.json"
        result = subprocess.run([args.tool, output, "--output", second, "--report", report,
                                 "--rules=capture", "--profile", profile, "--", *FLAGS],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.read_bytes(), second.read_bytes())
        self.assertFalse(json.loads(report.read_text())["edits"])

    def test_all_profiles_use_identical_mixed_capture_rule(self):
        code = r'''
struct Box { int n; };
int main() {
  Box box{1}; Box &alias = box;
  int n = 3, count = 0;
  int array[2] = {4, 5};
  auto callback = [n, &box, &alias, &count, &array]() mutable {
    box.n += n;
    alias.n += 2;
    ++count; array[1] += n;
    return ++n;
  };
  n = 20;
  int result = callback();
  return !(result == 4 && n == 20 && box.n == 6 && count == 1 && array[1] == 8);
}
'''
        outputs = []
        for profile in PASS_PROFILES:
            with self.subTest(profile=profile):
                self.clear_outputs()
                output, report = self.rewrite(code, "capture", profile=profile)
                self.assertTrue(report["edits"])
                self.assertIn("[n, boxPtr, aliasPtr, countPtr, arrayPtr]", output.read_text())
                outputs.append(output.read_text())
                self.second_round(output, profile)
        self.assertEqual(len(set(outputs)), 1)
        self.compare_behavior(output)

    def test_value_capture_keeps_copy_constructor_and_snapshot(self):
        output, _ = self.rewrite(r'''
int copies = 0;
struct Box {
  int n;
  Box(int n): n(n) {}
  Box(const Box &other): n(other.n) { ++copies; }
};
int main() {
  Box snapshot(3); int out = 0;
  auto callback = [snapshot, &out]() { out = snapshot.n; };
  snapshot.n = 9;
  callback();
  return !(copies == 1 && out == 3 && snapshot.n == 9);
}
''', "capture")
        self.assertIn("[snapshot, outPtr]", output.read_text())
        self.compare_behavior(output)
        self.second_round(output)

    def test_only_value_captures_are_unchanged(self):
        code = 'int main(){ int n=1; auto f=[n](){return n;}; n=2; return f()!=1; }'
        output, report = self.rewrite(code, "capture")
        self.assertFalse(report["edits"])
        self.assertEqual(output.read_text(), code)
        self.compare_behavior(output)

    def test_nested_this_fields_keep_instance_identity_in_every_profile(self):
        code = r'''
struct Runner {
  int axis;
  int run(int &value) {
    int bias = 3;
    auto callback = [this, &value, bias]() mutable {
      auto inner = [this, &value]() { axis += value; ++value; };
      inner(); return axis + ++bias;
    };
    int result = callback();
    return bias == 3 ? result : -1;
  }
};
int main() {
  Runner a{2}, b{8}; int x=1, y=2;
  return !(a.run(x)==7 && b.run(y)==14 && a.axis==3 && b.axis==10 && x==2 && y==3);
}
'''
        outputs = []
        for profile in PASS_PROFILES:
            with self.subTest(profile=profile):
                self.clear_outputs()
                output, _ = self.rewrite(code, "capture", profile=profile)
                outputs.append(output.read_text())
                self.second_round(output, profile)
        self.assertEqual(len(set(outputs)), 1)
        self.assertIn("&(this->axis)", output.read_text())
        self.assertIn("auto *axisPtr1 = axisPtr;", output.read_text())
        self.compare_behavior(output)

    def test_scalar_and_reference_only_are_candidates(self):
        output, report = self.rewrite(r'''
int main() {
  int n=1; int &ref=n;
  auto f=[&ref](){ ++ref; };
  f();
  return n!=2;
}
''', "capture")
        self.assertTrue(report["edits"])
        self.compare_behavior(output)
        self.second_round(output)

    def test_volatile_reference_preserves_qualified_access(self):
        output, _ = self.rewrite('int main(){ volatile int n=1; auto f=[&n](){++n;}; f(); return n!=2; }', "capture")
        self.compare_behavior(output)

    def test_reject_init_capture_in_mixed_lambda(self):
        _, report = self.rewrite('int main(){int n=1; auto f=[v=3,&n](){n+=v;}; f();}', "capture", 1)
        self.assertTrue(any("init/pack" in s for s in report["rejections"]))

    def test_reject_this_method_and_star_this(self):
        for capture, body in [("this", "run()"), ("*this, &n", "n++")]:
            with self.subTest(capture=capture):
                self.clear_outputs()
                self.rewrite('struct X { void run() {} void f(){int n=0; auto c=[' + capture + '](){' + body + ';}; c();} };', "capture", 1)

    def test_reject_copy_across_rewritten_reference(self):
        _, report = self.rewrite('int main(){int n=1; auto f=[&n](){auto g=[n](){return n;}; return g();}; return f()!=1;}', "capture", 1)
        self.assertTrue(any("value capture crosses" in s for s in report["rejections"]))

    def test_reject_short_circuit_hoist(self):
        self.rewrite('template<class F> bool call(F f){f(); return true;} int main(){int n=0; bool b=false && call([&n](){++n;}); return n;}', "capture", 1)

    def test_reject_decltype_auto_return(self):
        self.rewrite('int main(){int n=1; auto f=[&n]() -> decltype(auto) {return n;}; return f()!=1;}', "capture", 1)

    def test_reject_self_capture_before_declaration(self):
        _, report = self.rewrite('struct Store{template<class F>Store(F){}}; int main(){Store self([&self](){(void)&self;});}', "capture", 1)
        self.assertTrue(any("self capture" in s for s in report["rejections"]))

    def test_reject_goto_across_inserted_pointer(self):
        self.rewrite('template<class F> void call(F f){f();} int main(){int n=0; goto done; call([&n](){++n;}); done: return n;}', "capture", 1)


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
