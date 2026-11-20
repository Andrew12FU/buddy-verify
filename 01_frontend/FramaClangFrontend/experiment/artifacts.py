"""产物与追溯：只操作快照、补丁、哈希和 JSON，不解释 C++ 语义。"""

import difflib
import hashlib
import json
from pathlib import Path
import shutil

from .profiles import FRONTEND_ROOT, MINI_ROOT, RunConfig


def digest(path: Path | str) -> str:
    """读取 path 的字节并返回 SHA-256 十六进制值；读取失败抛 OSError。"""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save_json(path: Path, value: dict) -> None:
    """将 value 写为 UTF-8 可审查 JSON；path 应属于本轮目录，失败抛 OSError。"""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def snapshot_inputs(config: RunConfig) -> dict[str, str]:
    """复制输入到新输出目录，返回三个只读原件的路径→哈希。

    Args:
        config: 输出目录已经创建且通过非覆盖检查的运行配置。
    Returns:
        原始 pass、所选 mini 和人工 input.cpp 的哈希，供结束时再次核对。
        人工参考不参与规则生成，只用于独立验收。
    """
    shutil.copyfile(config.source, config.out / "original.cpp")
    originals = [config.source]
    if config.reference is not None:
        if not config.profile.reference_real_includes:
            originals.append(config.reference / "mini_mlir.h")
        else:
            originals.append(config.reference / "original.cpp")
    if config.profile.reference_source is not None:
        originals.append(config.profile.reference_source)
    if config.shared_mini:
        originals.extend(sorted(p for p in (MINI_ROOT / "include").rglob("*") if p.is_file()))
        originals.append(MINI_ROOT / "docs/interface-abstractions.json")
    else:
        shutil.copyfile(config.interface, config.out / "mini_mlir.h")
        originals.append(config.interface)
    return {str(p): digest(p) for p in originals}


def implementation_hashes() -> dict[str, str]:
    """返回当前全部实现/测试文件的相对路径→哈希。

    显式限定源码目录，排除 build/results/__pycache__，保证拆分后的新模块
    也进入追溯，而不会把历史输入、编译生成文件或日志误当作实现。
    """
    files = [FRONTEND_ROOT / "CMakeLists.txt", FRONTEND_ROOT / "run.py",
             FRONTEND_ROOT / "build_frontend.py"]
    # 逐项登记实验代码，避免递归纳入 interface_pilot/results 中的历史输入。
    for pattern in ["src/**/*.cpp", "include/**/*.h", "experiment/*.py", "tests/*.py", "tests/fixtures/nested_capture/*.cpp", "tests/references/*.cpp",
                    "experiment/interface_pilot/run.py",
                    "experiment/interface_pilot/python/*.py",
                    "experiment/interface_pilot/fixtures/*.cpp"]:
        files.extend(FRONTEND_ROOT.glob(pattern))
    result = {str(p.relative_to(FRONTEND_ROOT)): digest(p) for p in sorted(files)}
    for path in [MINI_ROOT / "extract.py", MINI_ROOT / "rewrite_headers.py",
                 *sorted((MINI_ROOT / "python").rglob("*.py"))]:
        result["../MiniMLIR/" + str(path.relative_to(MINI_ROOT))] = digest(path)
    return result


def write_baseline_and_patch(config: RunConfig, report: dict) -> None:
    """按 C++ 报告生成原始 mini 对照和可审查补丁。

    Args:
        config: 原始文件及本轮目录。
        report: C++ 输出的已接受计划，edits 按原始字节偏移递增排序。
    Returns:
        None；写 baseline_mini.cpp / rewrite.patch，不改任何原件。
    Note:
        Python 只重放 interface-substitution 编辑，不自行匹配或改写 C++。
        从后向前重放是为了保留尚未应用的原始字节偏移。
    """
    baseline = config.source.read_bytes()
    for edit in reversed(report["edits"]):
        if edit["rule"] == "interface-substitution":
            begin, length = edit.get("original_offset", edit["offset"]), edit["length"]
            baseline = baseline[:begin] + edit["after"].encode() + baseline[begin + length:]
    (config.out / "baseline_mini.cpp").write_bytes(baseline)
    (config.out / "rewrite.patch").write_text("".join(difflib.unified_diff(
        config.source.read_text().splitlines(keepends=True),
        (config.out / "rewritten.cpp").read_text().splitlines(keepends=True),
        fromfile=str(config.source), tofile=str(config.out / "rewritten.cpp"),
    )))


def restore_real_includes(config: RunConfig, report: dict) -> Path:
    """仅恢复已记录的 includes，创建真实依赖检查输入，不逆向其他规则。

    Args:
        config: 当前输出目录。
        report: 原始 C++ 报告，包含被替换的真实 include 原文。
    Returns:
        真实依赖检查的输入路径。共享模式直接返回 rewritten.cpp；旧单头
        文件模式临时生成 rewritten_real.cpp，检查结束后清理。
    Raises:
        RuntimeError: 无法唯一定位本轮加入的 mini 指令，拒绝猜测恢复位置。
    """
    rewritten = (config.out / "rewritten.cpp").read_bytes()
    if config.shared_mini:
        return config.out / "rewritten.cpp"
    directive = b'#include "mini_mlir.h"\n'
    original = b"".join(e["before"].encode() for e in report["edits"]
                        if e["rule"] == "interface-substitution")
    if rewritten.count(directive) != 1:
        raise RuntimeError("Cannot unambiguously restore real includes")
    (config.out / "rewritten_real.cpp").write_bytes(rewritten.replace(directive, original, 1))
    return config.out / "rewritten_real.cpp"


def compact_verified_artifacts(out: Path, summary: dict) -> None:
    """精简已完成检查的产物，并同步更新摘要中的日志引用。

    Args:
        out: 本轮受管理的输出目录，只处理固定名称的检查副本和阶段日志。
        summary: 可变阶段摘要；调用方必须已确认本轮要求的检查均通过。
    Returns:
        None。保留转换、原始转换对照和 C 回读日志到 logs/，版本文字
        合入摘要；其余成功阶段保留命令/退出码，不保留重复诊断文件。
    Note:
        不用于失败运行：显式指定目录的失败诊断应完整保留。
        检查副本的哈希仍记入摘要，但它们不再属于持久产物清单。
    """
    logs = out / "logs"
    logs.mkdir(exist_ok=True)
    important = {"frama_baseline_mini", "frama_rewritten", "frama_c_readback"}
    for name, stage in summary["stages"].items():
        value = stage.get("log")
        if not value:
            continue
        path = Path(value)
        if path.parent not in (out, logs):
            raise ValueError(f"Stage log outside the managed output: {path}")
        if name.endswith("version"):
            content = path.read_text(encoding="utf-8")
            summary.setdefault("tool_versions", {})[name] = content.split(
                "--- stdout ---\n", 1)[-1].split("\n--- stderr ---", 1)[0].strip()
        if name in important or stage["exit_code"] != 0:
            target = logs / path.name
            if path != target:
                path.rename(target)
            stage["log"] = str(target)
        else:
            path.unlink()
            stage["log"] = None
    removed = {}
    for name in ("rewritten_real.cpp", "second.cpp", "second.json",
                 "baseline_mini.cpp", "baseline.c"):
        path = out / name
        if path.is_file():
            removed[name] = digest(path)
            path.unlink()
    summary["artifact_retention"] = {
        "logs": "logs/",
        "routine_success_logs": "discarded; commands and exit codes remain in stages",
        "temporary_check_files_sha256": removed,
    }
