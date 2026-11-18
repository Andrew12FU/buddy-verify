"""规则测试公共夹具：独立输入、原件保护和编译执行对照。"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

parser = argparse.ArgumentParser()
parser.add_argument("--tool", required=True, type=Path)
parser.add_argument("--clang", required=True, type=Path)
args, rest = parser.parse_known_args()
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from experiment.toolchain import compiler_environment
os.environ.update(compiler_environment(args.clang.absolute().parents[1]))
# LibTooling 的独立可执行文件不在 LLVM bin 内，显式选择配套内建头目录。
resource = subprocess.run([str(args.clang), "-print-resource-dir"], capture_output=True,
                          text=True, check=True).stdout.strip()
FLAGS = ["-std=c++17", f"-resource-dir={resource}"]


class RewriteCase(unittest.TestCase):
    def setUp(self):
        """为当前用例建立独立临时目录，结束自动清理，避免旧产物串用。"""
        self.temp = tempfile.TemporaryDirectory(prefix="buddy-framac-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def rewrite(self, code, rules, expected=0, *, profile="reshape", options=()):
        """code 为原文，rules/profile/options 传给工具，expected 为预期退出码；返回 (输出路径, JSON 报告)，同时检查原件保护和拒绝时无输出。"""
        source = self.root / "source.cpp"
        source.write_text(code)
        output, report = self.root / "output.cpp", self.root / "report.json"
        command = [args.tool, source, "--output", output, "--report", report,
                   f"--rules={rules}", "--profile", profile, *options,
                   "--", *FLAGS]
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertEqual(result.returncode, expected, result.stderr)
        self.assertEqual(source.read_text(), code)
        if expected:
            self.assertFalse(output.exists())
        return output, json.loads(report.read_text())

    def compare_behavior(self, rewritten):
        """分别编译原文与 rewritten 并执行，均要求返回 0；失败成为断言，不代表完整 MLIR pass 已运行。"""
        for name, source in [("before", self.root / "source.cpp"), ("after", rewritten)]:
            executable = self.root / name
            compiled = subprocess.run([args.clang, "-std=c++17", source, "-o", executable],
                                      capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            ran = subprocess.run([executable], timeout=5)
            self.assertEqual(ran.returncode, 0, name)

