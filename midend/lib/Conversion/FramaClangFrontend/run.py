#!/usr/bin/env python3
"""运行已审计 pass 的自动改写及验证；本文件仅处理命令行和运行配置。

阅读阶段流程见 experiment/validation.py，C++ 规则见 src/rules/*Rule.cpp。原有参数保持兼容。
"""

import argparse
from pathlib import Path
import subprocess
import sys

from experiment.profiles import (CONVERSION_CIRGEN, CONVERSION_LLVM, FRONTEND_LLVM,
                                 FRONTEND_ROOT, FRONTEND_TOOL, PASS_PROFILES,
                                 MINI_ROOT, SYSTEM_PREFIX, WORKSPACE, RunConfig)
from experiment.validation import ExperimentRunner
from experiment.output import output_directory


def parse_config(argv: list[str] | None = None) -> RunConfig:
    """解析参数并建立运行配置；输出目录由成功替换策略管理。

    Args:
        argv: 显式参数列表；None 时使用进程命令行，便于独立测试入口。
    Returns:
        RunConfig，尚未创建/替换输出目录或运行工具。
    Raises:
        SystemExit(2): 必需文件缺失或目录非空，沿用 argparse 的错误语义。
        OSError: 目录创建失败，由 main 统一报告。
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pass", dest="pass_name", choices=PASS_PROFILES, default="reshape")
    parser.add_argument("--llvm-prefix", type=Path, default=FRONTEND_LLVM,
                        help="LLVM/Clang used by the frontend and native checks (default: 18)")
    parser.add_argument("--frama-llvm-prefix", type=Path, default=CONVERSION_LLVM,
                        help="LLVM/Clang paired with Frama-Clang (default: original 18)")
    parser.add_argument("--system-prefix", type=Path, default=SYSTEM_PREFIX,
                        help="Audited Conda sysroot/GCC C++ headers shared by both parsers")
    parser.add_argument("--tool", type=Path, default=FRONTEND_TOOL)
    parser.add_argument("--compile-commands", type=Path,
                        default=WORKSPACE / "build-v0.0.4/buddy/compile_commands.json")
    parser.add_argument("--interface", type=Path, help="Explicit legacy single-header model; default extracts shared MiniMLIR")
    parser.add_argument("--frama-c", type=Path, default=Path.home() / ".opam/framac-study/bin/frama-c")
    parser.add_argument("--frama-cirgen", type=Path, default=CONVERSION_CIRGEN,
                        help="Independent framaCIRGen binary (default: original Clang 18 build)")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--skip-frama", action="store_true", help="Explicitly leave conversion unverified")
    args = parser.parse_args(argv)
    profile = PASS_PROFILES[args.pass_name]
    llvm, tool = args.llvm_prefix.resolve(), args.tool.resolve()
    interface = args.interface.resolve() if args.interface else None
    out = args.output_dir.resolve() if args.output_dir else MINI_ROOT / "current" / profile.name
    if args.output_dir and out.exists() and (not out.is_dir() or any(out.iterdir())):
        parser.error("Explicit output directory must be new or empty")
    required = ([interface] if interface else [tool.parent / "buddy-mini-slice", MINI_ROOT / "extract.py"])
    for path in [tool, profile.source.resolve(), *required, args.compile_commands, llvm / "bin/clang++",
                 *([] if args.skip_frama else [args.frama_llvm_prefix / "bin/clang++"])]:
        if not path.is_file():
            parser.error(f"Required file is missing: {path}")
    return RunConfig(profile, llvm, tool, interface, args.compile_commands, args.frama_c,
                     out, args.skip_frama, args.frama_llvm_prefix.resolve(), args.system_prefix.resolve(),
                     args.frama_cirgen.resolve() if args.frama_cirgen else None,
                     replace_output=args.output_dir is None)


def main(argv: list[str] | None = None) -> int:
    """CLI 总入口。

    Args:
        argv: 可选参数列表，语义同 parse_config。
    Returns:
        0=所有要求阶段通过；1=初始化/实验失败；参数错误由 argparse 退出 2。
        初始化失败可能尚无摘要，阶段失败由 Runner 保留已完成的证据。
    """
    try:
        config = parse_config(argv)
        with output_directory(config.out, replace=config.replace_output):
            result = ExperimentRunner(config).run()
        if config.replace_output and (result or config.skip_frama):
            print("Current verified result retained; temporary run output discarded. "
                  "Use --output-dir only when diagnostics need to be saved.")
        return result
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"Failed to initialize experiment: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
