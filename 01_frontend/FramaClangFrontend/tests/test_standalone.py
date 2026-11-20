"""Standalone path/config regressions; no installed Buddy checkout required."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from run import parse_config
from experiment.profiles import PASS_PROFILES, REPO
from experiment.toolchain import compiler_arguments

class StandaloneTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="frontend cli ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.llvm = self.root / "llvm"
        self.tool = self.root / "build/buddy-framac-rewrite"
        for p in [self.llvm / "bin/clang++", self.tool, self.tool.parent / "buddy-mini-slice"]:
            p.parent.mkdir(parents=True, exist_ok=True); p.touch()
        self.args = ["--llvm-prefix", str(self.llvm), "--tool", str(self.tool),
                     "--skip-frama", "--output-dir", str(self.root / "new output")]

    def test_all_bundled_paths_are_repository_relative_and_exist(self):
        self.assertEqual(len(PASS_PROFILES), 9)
        for p in PASS_PROFILES.values():
            self.assertTrue(p.source.is_relative_to(REPO))
            self.assertTrue(p.source.is_file())
            self.assertTrue(p.reference_source.is_file())

    def test_constructor_is_opt_in_for_every_bundled_pass(self):
        for profile in PASS_PROFILES.values():
            self.assertNotIn("constructor", profile.rules.split(","))
            self.assertNotIn("constructor", profile.ast_rounds)
            self.assertNotIn("constructor", profile.expected)

    def test_preset_retains_audited_namespace_prefix_and_checks(self):
        c = parse_config([*self.args, "--pass", "matmul-scalar"])
        self.assertEqual(c.profile.namespace_prefix, "SimplifyTosaMatmulScalar")
        self.assertTrue(c.profile.require_reference_match)
        self.assertIsNotNone(c.profile.expected)
        self.assertEqual(c.frama_llvm, self.llvm.resolve())
        self.assertFalse(c.out.exists())

    def test_custom_input_has_own_name_and_no_bundled_reference(self):
        source = self.root / "user input.cpp"; source.write_text("int main() {}")
        c = parse_config([*self.args, "--input", str(source), "--rules", "namespace"])
        self.assertEqual(c.source, source)
        self.assertEqual(c.profile.namespace_prefix, "user input")
        self.assertIsNone(c.profile.expected)
        self.assertIsNone(c.reference)

    def test_real_mode_requires_explicit_database(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as e:
            parse_config([*self.args, "--parse-mode", "real"])
        self.assertEqual(e.exception.code, 2)

    def test_nonempty_output_is_preserved(self):
        out = self.root / "new output"; out.mkdir(); p = out / "keep"; p.write_text("keep")
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            parse_config(self.args)
        self.assertEqual(p.read_text(), "keep")

    def test_relative_database_source_and_output_flags(self):
        source = self.root / "source.cpp"; source.touch()
        db = self.root / "compile_commands.json"
        db.write_text(json.dumps([{"directory": ".", "file": "source.cpp",
             "arguments": ["clang++", "-std=c++17", "-c", "source.cpp", "-o", "danger.o"]}]))
        with patch("experiment.toolchain.resource_directory", return_value=self.llvm / "resource"):
            flags, cwd, entry = compiler_arguments(db, source, self.llvm)
        self.assertEqual(cwd, self.root)
        self.assertNotIn("-c", flags); self.assertNotIn("danger.o", flags)
        self.assertFalse(any("conda" in f for f in flags))

if __name__ == "__main__": unittest.main()
