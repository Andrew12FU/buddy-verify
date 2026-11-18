#!/usr/bin/env python3
"""为独立嵌套捕获样例保留原生执行、幂等性和 Frama-Clang 转换证据。

此入口使用自包含的 Box/回调样例；完整 MLIR pass 回归仍使用根目录 run.py。
默认使用自动清理的临时目录；显式 --output-dir 才保留诊断产物。
"""

import argparse
import json
from pathlib import Path
import shlex
import shutil
import sys
import tempfile

FRONTEND = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(FRONTEND))
from experiment.artifacts import digest, implementation_hashes, save_json
from experiment.profiles import CONVERSION_CIRGEN, CONVERSION_LLVM, FRONTEND_LLVM, FRONTEND_TOOL
from experiment.toolchain import (compiler_environment, conversion_arguments,
                                  resource_directory, run_command)


def run(argv: list[str]) -> int:
    """解析工具路径并逐个验证样例；返回 0=全部通过，1=阶段失败，参数错误为 2。

    阶段失败后保留 summary 和已完成日志；原始 Frama 对照的非零退出只记录。
    转换通过仅表明该自包含输入可转换，原生执行检查的是小样例的可观察结果。
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--llvm-prefix", type=Path, default=FRONTEND_LLVM)
    parser.add_argument("--frama-llvm-prefix", type=Path, default=CONVERSION_LLVM)
    parser.add_argument("--tool", type=Path, default=FRONTEND_TOOL)
    parser.add_argument("--frama-c", type=Path,
                        default=Path.home() / ".opam/framac-study/bin/frama-c")
    parser.add_argument("--frama-cirgen", type=Path, default=CONVERSION_CIRGEN)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args(argv)
    llvm, tool, frama = args.llvm_prefix.resolve(), args.tool.resolve(), args.frama_c.resolve()
    converter = (args.frama_cirgen or frama.parent / "framaCIRGen").resolve()
    frama_llvm = args.frama_llvm_prefix.resolve()
    for file in [tool, llvm / "bin/clang++", frama_llvm / "bin/clang++", frama, converter]:
        if not file.is_file():
            parser.error(f"Missing executable: {file}")
    out = args.output_dir.resolve()
    if out.exists() and (not out.is_dir() or any(out.iterdir())):
        parser.error("Output must be a new or empty directory")
    out.mkdir(parents=True, exist_ok=True)
    env = compiler_environment(llvm)
    frama_env = compiler_environment(frama_llvm, frama)
    flags = ["-std=c++17", "-target", "x86_64-conda-linux-gnu",
             f"-resource-dir={resource_directory(llvm)}", "-nostdinc++"]
    frama_flags = conversion_arguments(flags, frama_llvm)
    fixtures = sorted((FRONTEND / "tests/fixtures/nested_capture").glob("*.cpp"))
    if not fixtures:
        parser.error("No nested capture fixtures found")
    summary = {"schema_version": 1, "status": "in_progress", "cases": {},
               "original_sha256": {str(f): digest(f) for f in fixtures},
               "frontend_sources_sha256": implementation_hashes(),
               "tool_sha256": digest(tool), "versions": {},
               "toolchains": {"frontend": str(llvm), "conversion": str(frama_llvm),
                              "converter": str(converter)},
               "effective_arguments": flags, "effective_frama_arguments": frama_flags,
               "scope": "Self-contained nested captures; no MLIR implementation or lifetime proof"}
    current = None

    def stage(name: str, command: list, *, required: bool = True) -> None:
        """记录当前样例的独立阶段；required 时非零退出抛错，已有日志不删除。"""
        record = run_command(command, current / f"{name}.log", cwd=current,
                             env=frama_env if name.startswith("frama_") else env)
        summary["cases"][current.name]["stages"][name] = record
        save_json(out / "summary.json", summary)
        if required and record["exit_code"]:
            raise RuntimeError(f"{current.name}/{name}: exit {record['exit_code']}")

    try:
        for name, command in [("clang_version", [llvm / "bin/clang++", "--version"]),
                              ("frama_version", [frama, "-version"])]:
            version = run_command(command, out / f"{name}.log", cwd=out,
                                  env=frama_env if name.startswith("frama_") else env)
            summary["versions"][name] = version
            if version["exit_code"]:
                raise RuntimeError(f"Cannot read {name}")
        for fixture in fixtures:
            current = out / fixture.stem
            current.mkdir()
            shutil.copyfile(fixture, current / "original.cpp")
            summary["cases"][current.name] = {"stages": {}}
            stage("rewrite", [tool, current / "original.cpp", "--output", current / "rewritten.cpp",
                              "--report", current / "rewrite.json", "--rules=capture", "--", *flags])
            for version in ["original", "rewritten"]:
                stage(f"compile_{version}", [llvm / "bin/clang++", "-std=c++17",
                                              current / f"{version}.cpp", "-o", current / version])
                stage(f"execute_{version}", [current / version])
            stage("idempotence", [tool, current / "rewritten.cpp", "--output", current / "second.cpp",
                                  "--report", current / "second.json", "--rules=capture", "--", *flags])
            if ((current / "rewritten.cpp").read_bytes() != (current / "second.cpp").read_bytes()
                    or json.loads((current / "second.json").read_text())["edits"]):
                raise RuntimeError(f"{current.name}: rewrite is not idempotent")
            for version in ["original", "rewritten"]:
                stage(f"frama_{version}", [frama, current / f"{version}.cpp", "-cxx-nostdinc",
                      "-machdep", "gcc_x86_64", f"-cxx-clang-command={converter}",
                      "-fclang-cpp-extra-args=" + shlex.join(frama_flags), "-cxx-parseable-output", "-print",
                      "-ocode", current / f"{version}.c"], required=version == "rewritten")
            generated = current / "rewritten.c"
            if not generated.is_file() or not generated.stat().st_size:
                raise RuntimeError(f"{current.name}: missing converted C")
            summary["cases"][current.name]["status"] = "converted"
            summary["cases"][current.name]["artifacts_sha256"] = {
                file.name: digest(file) for file in current.iterdir() if file.is_file()
            }
        if any(digest(path) != value for path, value in summary["original_sha256"].items()):
            raise RuntimeError("An original fixture changed")
        summary["status"] = "converted"
        save_json(out / "summary.json", summary)
        print(f"converted: {out}")
        return 0
    except (RuntimeError, OSError, ValueError) as error:
        summary["status"], summary["error"] = "failed", str(error)
        save_json(out / "summary.json", summary)
        print(f"Failed: {error}\nEvidence: {out}", file=sys.stderr)
        return 1


def main() -> int:
    """Only explicit diagnostic paths persist; ordinary checks leave no archive."""
    argv = sys.argv[1:]
    if any(arg == "--output-dir" or arg.startswith("--output-dir=") for arg in argv):
        return run(argv)
    with tempfile.TemporaryDirectory(prefix="buddy-nested-capture-") as temporary:
        result = run([*argv, "--output-dir", temporary])
    print("Nested capture temporary output cleaned; use --output-dir to retain diagnostics.")
    return result


if __name__ == "__main__":
    raise SystemExit(main())
