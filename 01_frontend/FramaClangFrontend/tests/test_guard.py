"""Generic if + built-in || rewriting: behavior, scope and fresh-AST rounds."""

import json
import subprocess
import unittest

from rewrite_support import RewriteCase, args, rest, FLAGS


class GenericGuardTests(RewriteCase):
    def assert_stable(self, output, profile):
        second = self.root / "second.cpp"
        report = self.root / "second.json"
        result = subprocess.run(
            [str(args.tool), str(output), "--output", str(second), "--report",
             str(report), "--rules=guard", "--profile", profile, "--", *FLAGS],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(second.read_bytes(), output.read_bytes())
        self.assertFalse(json.loads(report.read_text())["edits"])

    def test_same_behavior_in_every_profile_and_loop(self):
        code = r'''
int calls;
bool check(bool b) { ++calls; return b; }
struct OrdinaryClass {
  int arbitraryMethod(bool a, bool b) {
    int result = 0;
    if (check(a) || check(b)) { result = 7; result += 2; }
    else result = 3;
    for (int i = 0; i < 2; ++i)
      if (a || b) result += 1; else result -= 1;
    return result;
  }
};
int main() {
  OrdinaryClass c;
  calls = 0; int x = c.arbitraryMethod(true, false), cx = calls;
  calls = 0; int y = c.arbitraryMethod(false, true), cy = calls;
  calls = 0; int z = c.arbitraryMethod(false, false), cz = calls;
  return !(x == 11 && y == 11 && z == 1 && cx == 1 && cy == 2 && cz == 2);
}
'''
        from experiment.profiles import PASS_PROFILES
        root = self.root
        try:
            for profile in PASS_PROFILES:
                with self.subTest(profile=profile):
                    self.root = root / profile
                    self.root.mkdir()
                    output, report = self.rewrite(code, "guard", profile=profile)
                    self.assertEqual(len(report["edits"]), 2)
                    self.assertIn("else result = 3;", output.read_text())
                    self.compare_behavior(output)
                    self.assert_stable(output, profile)
        finally:
            self.root = root

    def test_nested_and_else_if_converge_on_fresh_asts(self):
        code = r'''
int choose(bool a, bool b, bool c, bool d) {
  if (a || b) {
    if (c || d) return 11;
    return 12;
  } else if (c || d) return 13;
  else return 14;
}
int main() {
  return !(choose(true,false,false,true) == 11 &&
           choose(false,true,false,false) == 12 &&
           choose(false,false,true,false) == 13 &&
           choose(false,false,false,false) == 14);
}
'''
        output, report = self.rewrite_to_fixpoint(code, "guard", profile="reshape")
        self.assertEqual(len(report["edits"]), 3)
        self.assertGreater(sum(r["edit_count"] > 0 for r in report["rounds"]), 1)
        self.assertEqual(report["rounds"][-1]["edit_count"], 0)
        self.compare_behavior(output)
        self.assert_stable(output, "reshape")

    def test_if_initializer_lifetime_and_expression(self):
        code = r'''
int alive, created, destroyed, count;
struct Scope {
  int n;
  Scope(int n) : n(n) { ++alive; ++created; }
  Scope(const Scope &) = delete;
  ~Scope() { --alive; ++destroyed; }
};
int choose(bool a, bool b) {
  int result;
  if (Scope s{7}; a || b) { result = s.n + 100 * (alive - 1); }
  else result = s.n + 1 + 100 * (alive - 1);
  return result + 100 * alive;
}
int expression(bool a, bool b) {
  if (++count; a || b) return 3;
  else return 4;
}
int main() {
  int x = choose(true,false), y = choose(false,false);
  int p = expression(true,false), q = expression(false,false);
  return !(x == 7 && y == 8 && created == 2 && destroyed == 2 &&
           alive == 0 && count == 2 && p == 3 && q == 4);
}
'''
        output, report = self.rewrite(code, "guard", profile="lower-ime")
        self.assertEqual(len(report["edits"]), 2)
        self.compare_behavior(output)
        self.assert_stable(output, "lower-ime")

    def test_condition_temporaries_are_cleaned_before_either_branch(self):
        code = r'''
int alive, made, converted;
struct Flag {
  bool value;
  Flag(bool value) : value(value) { ++alive; ++made; }
  ~Flag() { --alive; }
  explicit operator bool() const { ++converted; return value; }
};
int choose(bool a, bool b) {
  if (Flag{a} || Flag{b}) return 7 + 100 * alive;
  else return 8 + 100 * alive;
}
int main() {
  int x = choose(true,false), y = choose(false,true), z = choose(false,false);
  return !(x == 7 && y == 7 && z == 8 && made == 5 && converted == 5 && alive == 0);
}
'''
        output, report = self.rewrite(code, "guard")
        self.assertEqual(len(report["edits"]), 1)
        self.compare_behavior(output)

    def test_lambda_inside_condition_is_deferred_without_overlap(self):
        code = r'''
int choose(bool a, bool b) {
  if ([a]() { if (a || false) return true; return false; }() || b)
    return 7;
  else return 8;
}
int main() {
  return !(choose(true,false) == 7 && choose(false,true) == 7 &&
           choose(false,false) == 8);
}
'''
        output, report = self.rewrite_to_fixpoint(code, "guard", profile="reshape")
        self.assertEqual(len(report["edits"]), 2)
        self.compare_behavior(output)
        self.assert_stable(output, "reshape")

    def test_branch_literal_bytes_and_comments_are_preserved(self):
        code = r'''
int choose(bool a, bool b) {
  if (a || b) {
    // keep then
    const char *text = R"(x
  y)";
    return text[2] == ' ' ? 7 : 99;
  } else {
    // keep else
    const char *text = R"(p
    q)";
    return text[2] == ' ' ? 8 : 99;
  }
}
int main() { return !(choose(true,false) == 7 && choose(false,false) == 8); }
'''
        output, _ = self.rewrite(code, "guard")
        for literal in ['R"(x\n  y)"', 'R"(p\n    q)"', '// keep then', '// keep else']:
            self.assertIn(literal, output.read_text())
        self.compare_behavior(output)

    def test_overloaded_or_and_constexpr_are_untouched(self):
        code = r'''
struct Flag {};
bool operator||(Flag, Flag) { return true; }
int choose(Flag a, Flag b) {
  if (a || b) return 7; else return 8;
}
template<class T> int constant() {
  if constexpr (sizeof(T) == 1 || sizeof(T) == 2) return 1;
  else return T::missing;
}
int main() { return !(choose({}, {}) == 7 && constant<char>() == 1); }
'''
        output, report = self.rewrite(code, "guard")
        self.assertFalse(report["edits"])
        self.assertEqual(output.read_text(), code)
        self.compare_behavior(output)

    def test_macro_and_label_scopes_are_rejected(self):
        for name, code in {
            "macro": '#define C (a || b)\nvoid f(bool a,bool b) { if(C) {} }',
            "label": 'void f(bool a,bool b) { if(a || b) { label:; } }',
        }.items():
            root = self.root
            with self.subTest(name=name):
                self.root = root / name
                self.root.mkdir()
                try:
                    _, report = self.rewrite(code, "guard", expected=1)
                    self.assertTrue(report["rejections"])
                finally:
                    self.root = root


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
