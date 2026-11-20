#!/usr/bin/env python3
"""Exercise the linked parser version and newer parser capability on real inputs."""

import unittest

from rewrite_support import RewriteCase, args, rest
from experiment.toolchain import compiler_major, verify_parser_version

MAJOR = compiler_major(args.clang.absolute().parents[1])


class ParserVersionTests(RewriteCase):
    def test_linked_parser_matches_native_compiler(self):
        output, report = self.rewrite(
            f"static_assert(__clang_major__ == {MAJOR});\n"
            "namespace { struct Item {}; }\nint main() { Item item; return 0; }\n",
            "namespace")
        verify_parser_version(report, args.clang.absolute().parents[1])
        self.compare_behavior(output)

    @unittest.skipUnless(MAJOR == 23, "Clang 23 migration capability check")
    def test_new_builtin_parses_and_preserves_behavior(self):
        # Clang 18 rejects this builtin. Conversion through Frama-Clang 18 would
        # require an additional, independently audited adaptation.
        output, report = self.rewrite("""
static_assert(__has_builtin(__builtin_popcountg));
namespace {
struct Counter {
  static int count(unsigned value) { return __builtin_popcountg(value); }
};
}
int main() { return Counter::count(7u) != 3; }
""", "namespace")
        verify_parser_version(report, args.clang.absolute().parents[1])
        self.compare_behavior(output)


if __name__ == "__main__":
    unittest.main(argv=[__file__, *rest])
