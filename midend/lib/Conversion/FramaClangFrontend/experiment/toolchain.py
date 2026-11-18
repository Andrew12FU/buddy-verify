"""工具链边界：参数准备、进程环境、外部调用与原样日志。

本层不匹配 C++ 结构、不决定某条规则是否正确，也不把退出 0 当成证明。
"""

import json
import os
import re
from pathlib import Path
import shlex
import subprocess
from typing import Sequence

from .profiles import SYSTEM_PREFIX, RunConfig


def compiler_environment(llvm: Path, frama_c: Path | None = None) -> dict[str, str]:
    """Build a per-process environment; never mix the 18 and 23 library paths."""
    env = os.environ.copy()
    bins = [llvm / "bin"]
    libraries = [p for p in [llvm / "lib", llvm.parent / "x86_64-linux-gnu"] if p.is_dir()]
    if frama_c is not None:
        bins.insert(0, frama_c.parent)
        libraries.insert(0, frama_c.parent.parent / "lib/stublibs")
    env["PATH"] = os.pathsep.join(map(str, bins)) + os.pathsep + env.get("PATH", "")
    env["LD_LIBRARY_PATH"] = os.pathsep.join(map(str, libraries))
    return env


def resource_directory(llvm: Path) -> Path:
    """Ask the selected compiler for its actual builtin-header directory."""
    result = subprocess.run([str(llvm / "bin/clang++"), "-print-resource-dir"],
                            env=compiler_environment(llvm), capture_output=True,
                            text=True, check=True)
    path = Path(result.stdout.strip())
    if not path.is_dir():
        raise ValueError(f"Missing Clang resource directory: {path}")
    return path


def compiler_major(llvm: Path) -> int:
    """Query the selected native compiler to detect a stale frontend binary."""
    result = subprocess.run([str(llvm / "bin/clang++"), "-dumpversion"],
                            env=compiler_environment(llvm), capture_output=True,
                            text=True, check=True)
    return int(result.stdout.strip().split(".")[0])


def verify_parser_version(report: dict, llvm: Path) -> None:
    """Reject a tool linked to a different Clang major than its builtin headers."""
    match = re.search(r"clang version (\d+)\.", report.get("clang_version", ""))
    expected = compiler_major(llvm)
    if not match or int(match.group(1)) != expected:
        raise ValueError(f"Parser/native compiler version mismatch: {report.get('clang_version')}; expected {expected}")


def verify_converter_version(version_output: str, llvm: Path) -> None:
    """The translator's linked LLVM must match its selected builtin headers."""
    match = re.search(r"LLVM version (\d+)\.", version_output)
    expected = compiler_major(llvm)
    if not match or int(match.group(1)) != expected:
        raise ValueError(f"Converter/native compiler version mismatch; expected LLVM {expected}")


def conversion_arguments(flags: list[str], llvm: Path) -> list[str]:
    """Keep source/standard-library flags, selecting Frama-Clang's builtin headers."""
    result = []
    skip = False
    for flag in flags:
        if skip:
            skip = False
        elif flag == "-resource-dir":
            skip = True
        elif not flag.startswith("-resource-dir="):
            result.append(flag)
    return [*result, f"-resource-dir={resource_directory(llvm)}"]


def mini_arguments(flags: list[str], include: Path) -> list[str]:
    """Isolate model lookup from real project/system include search paths.

    Keep target, language and feature definitions; replace search paths so a
    missing mini header fails instead of silently importing the full MLIR tree.
    The shared model supplies its own bounded standard declarations.
    """
    result, skip = [], False
    for flag in flags:
        if skip:
            skip = False
        elif flag in {"-I", "-isystem", "-iquote", "-idirafter"}:
            skip = True
        elif not any(flag.startswith(prefix) for prefix in
                     ("-I", "-isystem", "-iquote", "-idirafter")):
            result.append(flag)
    return ["-I" + str(include), *result]


def compiler_arguments(database: Path, source: Path, llvm: Path,
                       system_prefix: Path = SYSTEM_PREFIX) -> tuple[list[str], Path, dict]:
    """从真实构建条目提取用于解析/语法检查的参数。

    Args:
        database: compile_commands.json 路径，只读。
        source: 当前 pass 的原始主文件，必须恰好匹配一个条目。
        llvm: 当前解析器/原生检查的 LLVM 前缀，内建头由编译器查询。
        system_prefix: 独立的已审计 Conda sysroot/GCC 标准库前缀。
    Returns:
        (有效参数列表, 原始工作目录, 未修改的数据库条目)。移除编译动作、
        对象/依赖输出路径和原输入参数，以免覆盖真实构建产物。
    Raises:
        ValueError: 条目不唯一或 GCC 标准头根目录不唯一。
        OSError: 输入读取失败。
    """
    entries = json.loads(database.read_text())
    matches = [e for e in entries if Path(e["file"]).resolve() == source.resolve()]
    if len(matches) != 1:
        raise ValueError(f"Expected exactly one compilation database entry for {source.name}")
    entry = matches[0]
    argv = entry.get("arguments") or shlex.split(entry["command"])
    arguments = []
    skip = False
    for arg in argv[1:]:
        if skip:
            skip = False
            continue
        if arg in {"-o", "-MF", "-MT", "-MQ", "-MJ"}:
            skip = True
        elif arg not in {"-c", "-MD", "-MMD", "-MP", "-fsyntax-only"}:
            candidate = Path(entry["directory"]) / arg
            if candidate.resolve() != source.resolve():
                arguments.append(arg)
    roots = list((system_prefix / "lib/gcc/x86_64-conda-linux-gnu").glob("*/include/c++"))
    if len(roots) != 1:
        raise ValueError("Expected one audited GCC C++ header root in LLVM prefix")
    root = roots[0]
    # 这些参数固定当前实验环境，不是跨平台自动探测或任意选项清洗器。
    arguments += [
        "-target", "x86_64-conda-linux-gnu", "-fno-color-diagnostics",
        f"--sysroot={system_prefix / 'x86_64-conda-linux-gnu/sysroot'}",
        f"-resource-dir={resource_directory(llvm)}", "-nostdinc++",
        "-isystem", str(root), "-isystem", str(root / "x86_64-conda-linux-gnu"),
        "-isystem", str(root / "backward"),
    ]
    return arguments, Path(entry["directory"]), entry


def create_environment(config: RunConfig, *, conversion: bool = False) -> dict[str, str]:
    """返回为当前工具链准备的子进程环境副本，不修改宿主 os.environ。

    Args:
        config: 两套独立 LLVM 前缀及 Frama-C 安装位置。
        conversion: True 选择转换器的配套库、工具和 OCaml stubs。
    Returns:
        包含工具搜索路径和动态库路径的环境；不会记录无关环境变量。
    """
    if conversion:
        return compiler_environment(config.frama_llvm, config.frama_c)
    return compiler_environment(config.llvm)


def run_command(command: Sequence[str | Path], log: Path, *, cwd: Path,
                env: dict[str, str], timeout: int = 180) -> dict:
    """运行一个无 shell 的命令并保留完整诊断，失败退出码也正常返回。

    Args:
        command: argv 序列；日志中的 shlex.join 只用于显示，不交给 shell 执行。
        log: 本轮输出目录中的日志文件。
        cwd: 子进程的工作目录，沿用真实编译条目。
        env: create_environment 返回的环境副本。
        timeout: 秒数；超时统一记录为退出 124。
    Returns:
        含 command/cwd/exit_code/log 的阶段记录。非零退出不抛异常，
        因为原始 mini 的转换失败也是需要保存的对照结果。
    Raises:
        OSError: 工具无法启动或日志无法写入；由流程层记录失败。
    """
    command = [str(arg) for arg in command]
    try:
        result = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                                text=True, errors="replace", timeout=timeout)
        code, stdout, stderr = result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired as error:
        code, stdout, stderr = 124, error.stdout or b"", error.stderr or b""
        if isinstance(stdout, bytes):
            stdout = stdout.decode(errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode(errors="replace")
        stderr += f"\nTimed out after {timeout} seconds\n"
    log.write_text(
        f"$ {shlex.join(command)}\ncwd: {cwd}\nexit_code: {code}\n"
        f"--- stdout ---\n{stdout}\n--- stderr ---\n{stderr}", encoding="utf-8",
    )
    return {"command": command, "cwd": str(cwd), "exit_code": code, "log": str(log)}


def frama_command(config: RunConfig, flags: list[str], input_name: str,
                  output_name: str) -> list[str | Path]:
    """只构造转换命令，不启动进程。

    Args:
        config: 当前运行配置，决定输出目录及 Frama-C 安装。
        flags: 转换器专用有效 C++ 参数；内建头必须与其配套 Clang 一致。
        input_name, output_name: 当前输出目录内的输入和 C 产物文件名。
    Returns:
        Frama-C 的 argv；shlex.join 将多个 C++ 参数编码为该选项的单个值。
        实际调用仍由 run_command 以 shell=False 执行。
    """
    return [config.frama_c, config.out / input_name, "-cxx-nostdinc",
            "-machdep", "gcc_x86_64",
            f"-cxx-clang-command={config.converter}",
            "-fclang-cpp-extra-args=" + shlex.join(flags),
            "-cxx-parseable-output", "-print", "-ocode", config.out / output_name]
