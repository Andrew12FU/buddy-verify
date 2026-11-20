"""实验流程层：固定验收顺序，分别保留解析、改写和转换结论。"""

from collections import Counter
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Sequence

from .artifacts import (compact_verified_artifacts, digest, implementation_hashes, restore_real_includes,
                        save_json, snapshot_inputs, write_baseline_and_patch)
from .profiles import MINI_ROOT, REPO, RunConfig
from .inventory import write_inventory
from .interface_plans import merge_dependency_roots
from .toolchain import (compiler_arguments, conversion_arguments, create_environment,
                        frama_command, mini_arguments, run_command, verify_parser_version)
from .toolchain import verify_converter_version


def repository_head():
    try:
        result = subprocess.run(["git", "-C", str(REPO), "rev-parse", "HEAD"],
                                capture_output=True, text=True)
        return result.stdout.strip() if result.returncode == 0 else None
    except FileNotFoundError:
        return None


class ExperimentRunner:
    """持有一轮实验状态，不持有 AST，也不实现 C++ 改写。

    从 run() 开始审查阶段顺序，再进入相应方法。所有产物在 config.out 内，
    固定结果的成功替换/失败恢复由命令行入口负责。
    """

    def __init__(self, config: RunConfig):
        """准备参数、环境和输入快照。

        Args:
            config: 已通过文件检查、输出目录已经创建的只读运行配置。
        Raises:
            OSError, ValueError, subprocess.SubprocessError: 初始化失败，此时
            摘要可能尚未建立，由入口报告，不伪造已执行的阶段。
        """
        self.config = config
        if config.parse_mode == "real":
            self.flags, self.cwd, entry = compiler_arguments(config.database,
                config.compile_source or config.source, config.llvm, config.system_prefix)
            self.flags += ["-iquote", str((config.compile_source or config.source).parent), *config.extra_arguments]
        else:
            from .toolchain import resource_directory
            self.flags = ["-std=c++17", "-I" + str(MINI_ROOT / "include"),
                          "-resource-dir=" + str(resource_directory(config.llvm)), *config.extra_arguments]
            self.cwd, entry = config.source.parent, None
        self.frama_flags = (conversion_arguments(self.flags, config.frama_llvm)
                            if not config.skip_frama else None)
        self.env = create_environment(config)
        self.frama_env = create_environment(config, conversion=True)
        self.originals = snapshot_inputs(config)
        self.report: dict = {}
        self.summary = {
            "schema_version": 1, "pass": config.profile.name, "status": "in_progress",
            "source": str(config.source),
            "interface": str(MINI_ROOT / "include") if config.shared_mini else str(config.interface),
            "interface_mode": "shared_member_extraction" if config.shared_mini else "legacy_single_header",
            "reference": str(config.reference), "rules": config.profile.rules,
            "expected_rewrite_counts": {k: v for k, v in (config.profile.expected or {}).items()
                                        if not config.shared_mini or k != "interface-substitution"},
            "compatibility_notes": list(config.profile.compatibility_notes),
            "original_sha256": self.originals, "compile_database_entry": entry,
            "tool_sha256": digest(config.tool),
            "frontend_sources_sha256": implementation_hashes(),
            "repository_head": repository_head(),
            "parse_mode": config.parse_mode,
            "real_dependencies_checked": config.parse_mode == "real",
            "bundled_audit_checks": config.profile.expected is not None,
            "effective_arguments": self.flags, "stages": {},
            "effective_frama_arguments": self.frama_flags,
            "toolchains": {"frontend": str(config.llvm), "conversion": str(config.frama_llvm),
                           "system_headers": str(config.system_prefix) if config.system_prefix else "compiler defaults",
                           "converter": str(config.converter)},
            "limits": ["Bounded shared library member extraction; not arbitrary MLIR header extraction"
                       if config.shared_mini else "Legacy interface model explicitly selected",
                       "Namespace naming changes internal linkage",
                       "Conversion success is not a proof of semantic equivalence"],
        }

    def save(self) -> None:
        """保存当前摘要到本轮 summary.json；写入失败抛 OSError。"""
        save_json(self.config.out / "summary.json", self.summary)

    def record_inventory(self) -> None:
        """分开记录已应用的源码/头文件规则；失败也保留已有改写证据。"""
        inventory = write_inventory(self.config.out, self.config.profile.name, self.summary["status"])
        if inventory is not None:
            self.summary["rewrite_inventory"] = inventory

    def stage(self, name: str, command: Sequence[str | Path], *, conversion: bool = False) -> bool:
        """执行并记录一个阶段。

        Args:
            name: 日志文件名及 summary.stages 的键。
            command: 无 shell 的 argv 序列。
        Returns:
            命令退出 0 时为 True；不自动意味着断言有效或语义等价。
            调用方可以接受对照的非零退出码，其日志仍完整保留。
        """
        logs = self.config.out / "logs"
        logs.mkdir(exist_ok=True)
        result = run_command(command, logs / f"{name}.log", cwd=self.cwd,
                             env=self.frama_env if conversion else self.env)
        self.summary["stages"][name] = result
        self.save()
        return result["exit_code"] == 0

    def rewrite(self) -> None:
        """调用 C++，核对 profile/编辑计数/人工参考，再生成对照和补丁。

        成功时填充 self.report；验收不通过抛 RuntimeError。Python 只重放
        已记录的 include 编辑，不自行匹配 C++。计数和参考是独立验收条件。
        """
        c, p = self.config, self.config.profile
        substitution = [] if c.shared_mini else ["--interface-header", "mini_mlir.h"]
        if p.ast_rounds:
            self.report = self.rewrite_rounds()
            save_json(c.out / "rewrite.json", self.report)
        else:
            if not self.stage("rewrite", [c.tool, c.source, "--output", c.out / "rewritten.cpp",
                                          "--report", c.out / "rewrite.json", "--profile", p.name,
                                          "--rules", p.rules, "--namespace-prefix", p.namespace_prefix or c.source.stem, *substitution,
                                          "--", *self.flags]):
                raise RuntimeError("C++ parsing or rewrite rejected; inspect logs/rewrite.log and rewrite.json")
            self.report = json.loads((c.out / "rewrite.json").read_text())
        verify_parser_version(self.report, c.llvm)
        if self.report.get("profile") != p.name:
            raise RuntimeError("Frontend profile differs from the selected pass")
        actual = Counter(edit["rule"] for edit in self.report["edits"])
        self.summary["rewrite_counts"] = dict(actual)
        expected = Counter(self.summary["expected_rewrite_counts"])
        if p.expected is not None and actual != expected:
            raise RuntimeError(f"{p.name} rule coverage changed: {dict(actual)}; review before proceeding")
        if p.reference is None:
            self.summary["reference_source_byte_equal"] = None
            write_baseline_and_patch(c, self.report)
            self.record_inventory()
            return
        reference_path = p.reference_source or c.reference / "input.cpp"
        self.summary["reference_source"] = str(reference_path)
        reference = reference_path.read_bytes()
        if c.shared_mini and not p.reference_real_includes:
            # Compare against the historical reference after restoring only its
            # include block. This never edits C++ statements or the original file.
            includes = b"".join(line for line in c.source.read_bytes().splitlines(keepends=True)
                                if line.lstrip().startswith(b"#include"))
            directive = b'#include "mini_mlir.h"\n'
            if reference.count(directive) != 1:
                raise RuntimeError("Historical reference has no unique mini include")
            reference = reference.replace(directive, includes, 1)
        equal = (c.out / "rewritten.cpp").read_bytes() == reference
        self.summary["reference_source_byte_equal"] = equal
        if p.require_reference_match and not equal:
            raise RuntimeError("Selected pass differs from its audited manual reference; review the full patch")
        write_baseline_and_patch(c, self.report)
        self.record_inventory()

    def rewrite_rounds(self) -> dict:
        """Run rules on fresh real-header ASTs, retaining a single result.

        Enclosing lambda/if/range replacements use different coordinate spaces.
        Each edit therefore records its round and that round's input hash;
        intermediate C++ files live only in a temporary directory.
        Returns the aggregate report. No C++ matching occurs in Python.
        """
        c, p = self.config, self.config.profile
        edits, rounds, report = [], [], None
        with tempfile.TemporaryDirectory(prefix="buddy-rules-") as directory:
            root = Path(directory)
            current = c.source
            for rule in p.ast_rounds:
                for attempt in range(32):
                    n = len(rounds)
                    output, detail = root / f"round-{n}.cpp", root / f"round-{n}.json"
                    if not self.stage(f"rewrite_{rule}_{attempt}", [c.tool, current,
                            "--output", output, "--report", detail, "--profile", p.name,
                            "--namespace-prefix", p.namespace_prefix or c.source.stem, "--rules", rule,
                            "--", *self.flags]):
                        if detail.is_file():
                            (c.out / "rewrite.json").write_bytes(detail.read_bytes())
                        raise RuntimeError(f"Rule {rule} rejected")
                    report = json.loads(detail.read_text())
                    phases = sorted({edit["rule"] for edit in report["edits"]})
                    rounds.append({"rule": rule, "phases": phases, "input_sha256": digest(current),
                                   "output_sha256": digest(output), "edit_count": len(report["edits"])})
                    for edit in report["edits"]:
                        edits.append({**edit, "round": n, "input_sha256": digest(current)})
                    current = output
                    if rule not in {"if-condition", "guard", "init-list"} or not report["edits"]:
                        break
                else:
                    raise RuntimeError(f"Rule {rule} did not converge after 32 fresh-AST rounds")
            if not c.shared_mini:
                # Keep real includes through every AST round. Apply the legacy
                # include view only after all source rules have converged.
                n = len(rounds)
                output, detail = root / f"round-{n}.cpp", root / f"round-{n}.json"
                if not self.stage("rewrite_interface_substitution", [c.tool, current,
                        "--output", output, "--report", detail, "--profile", p.name,
                        "--namespace-prefix", p.namespace_prefix or c.source.stem, "--rules", p.rules,
                        "--interface-header", "mini_mlir.h", "--", *self.flags]):
                    raise RuntimeError("Final interface substitution rejected")
                report = json.loads(detail.read_text())
                original = c.source.read_bytes()
                cursor = 0
                for edit in report["edits"]:
                    if edit["rule"] != "interface-substitution":
                        raise RuntimeError("Source rules changed after AST rounds converged")
                    # The detailed offset still refers to this final round.
                    # Baseline replay needs the corresponding original offset.
                    before = edit["before"].encode()
                    at = original.find(before, cursor)
                    if at < 0:
                        raise RuntimeError("Cannot map legacy include edit to the original source")
                    cursor = at + len(before)
                    edits.append({**edit, "round": n, "input_sha256": digest(current),
                                  "original_offset": at})
                rounds.append({"rule": "interface-substitution",
                               "phases": ["interface-substitution"] if report["edits"] else [],
                               "input_sha256": digest(current), "output_sha256": digest(output),
                               "edit_count": len(report["edits"])})
                current = output
            (c.out / "rewritten.cpp").write_bytes(current.read_bytes())
        return {**report, "input": str(c.source), "output": str(c.out / "rewritten.cpp"),
                "rules": list(p.rules.split(",")), "edits": edits, "rounds": rounds,
                "offset_basis": "UTF-8 bytes in the input of each numbered AST round"}

    def extract_interfaces(self) -> None:
        """Parse against the sole shared model, then delete unneeded declarations.

        The native AST selects roots and computes structural/overload closure.
        Python only materializes owned header bytes. No pass-specific C++ model
        definitions or user-source rewrites are created by this stage.
        """
        c = self.config
        if not c.shared_mini:
            return
        flags = mini_arguments(self.flags, MINI_ROOT / "include")
        if not self.stage("interface_slice", [c.slicer, c.out / "rewritten.cpp",
                                              "--library-root", MINI_ROOT / "include",
                                              "--report", c.out / "interface-plan.json", "--", *flags]):
            raise RuntimeError("Shared mini parsing or dependency extraction failed")
        if c.profile.ast_rounds:
            # Source desugaring removes language helpers such as enumerate.
            # Keep the original roots too, so the dedicated mini still supports
            # both versions rather than silently weakening the baseline check.
            with tempfile.TemporaryDirectory(prefix="buddy-interface-roots-") as directory:
                original_plan = Path(directory) / "original-plan.json"
                if not self.stage("interface_slice_original", [c.slicer, c.source,
                        "--library-root", MINI_ROOT / "include", "--report", original_plan,
                        "--", *flags]):
                    raise RuntimeError("Original-source interface dependency extraction failed")
                merge_dependency_roots(c.out / "interface-plan.json", original_plan)
        if not self.stage("interface_materialize", [sys.executable, MINI_ROOT / "extract.py",
                    "--source", c.out / "rewritten.cpp", "--plan", c.out / "interface-plan.json",
                    "--output", c.mini_include]):
            raise RuntimeError("Cannot materialize the pass member library")
        manifest = json.loads((c.out / "interface-manifest.json").read_text())
        self.summary["interface_extraction"] = {
            "canonical_root": str(MINI_ROOT / "include"), "generated_root": str(c.mini_include),
            "header_count": len(manifest["generated_headers_sha256"]),
            "removed_declaration_count": len(manifest["removed"]),
            "retained_declaration_count": len(manifest["retained"]),
            "slicer_sha256": digest(c.slicer),
        }
        self.save()

    def native_checks(self) -> None:
        """依次检查原始 mini、改写 mini 和改写真实依赖；不链接或执行。

        Returns:
            None。任一非零退出抛 RuntimeError，已完成阶段的日志保留。
        """
        c = self.config
        flags = mini_arguments(self.flags, c.mini_include) if c.shared_mini else self.flags
        for name, file, error in [
            ("native_baseline_mini", "baseline_mini.cpp", "Original Buddy code does not parse against the supplied interface"),
            ("native_rewritten_mini", "rewritten.cpp", "Rewritten code failed native syntax/semantic checking"),
        ]:
            if not self.stage(name, [c.llvm / "bin/clang++", *flags, "-fsyntax-only", c.out / file]):
                raise RuntimeError(error)
        if c.parse_mode != "real":
            self.summary["stages"]["native_rewritten_real"] = {
                "status": "not_run", "reason": "Model mode; external real dependencies were not supplied"}
            self.save()
            return
        real_source = restore_real_includes(c, self.report)
        if not self.stage("native_rewritten_real", [c.llvm / "bin/clang++", *self.flags,
                                                    "-fsyntax-only", real_source]):
            raise RuntimeError("Rewritten code failed against real dependencies")

    def rewrite_headers(self) -> None:
        """从专用库生成独立的转换用副本；源码 pass 与共享库均不修改。

        Python 规则只接受已审计的 MiniMLIR 模板形状；每条改写保留来源和差异。
        两份库都检查原始/改写源码，防止把转换兼容性混同于原接口匹配。
        返回 None；生成或语法检查失败时保留日志并抛 RuntimeError。
        """
        c = self.config
        if not c.shared_mini:
            return
        if not self.stage("header_rewrite", [sys.executable, MINI_ROOT / "rewrite_headers.py",
                "--source", c.mini_include, "--manifest", c.out / "interface-manifest.json",
                "--output", c.conversion_include]):
            raise RuntimeError("MiniMLIR header compatibility rewrite failed")
        report = json.loads((c.out / "header-rewrite.json").read_text())
        self.summary["header_rewrites"] = {
            "rules": dict(Counter(edit["rule"] for edit in report["edits"])),
            "conversion_include": str(c.conversion_include),
            "report_sha256": digest(c.out / "header-rewrite.json"),
        }
        self.record_inventory()
        flags = mini_arguments(self.flags, c.conversion_include)
        for name, file in [("native_baseline_compatible", "baseline_mini.cpp"),
                           ("native_rewritten_compatible", "rewritten.cpp")]:
            if not self.stage(name, [c.llvm / "bin/clang++", *flags, "-fsyntax-only", c.out / file]):
                raise RuntimeError("Header compatibility copy failed native checking")
        if not c.skip_frama:
            flags18 = mini_arguments(self.frama_flags, c.conversion_include)
            if not self.stage("native_conversion_compatible", [c.frama_llvm / "bin/clang++",
                    *flags18, "-fsyntax-only", c.out / "rewritten.cpp"], conversion=True):
                raise RuntimeError("Header compatibility copy failed conversion compiler checking")
        self.save()

    def idempotence_check(self) -> None:
        """使用相同 profile/rules 再次改写并要求源码字节不变。

        返回 None，失败抛 RuntimeError。稳定性检查不能替代语义检查；
        检查结果写入摘要；成功后删除 second.cpp / second.json。
        """
        c, p = self.config, self.config.profile
        flags = mini_arguments(self.flags, c.mini_include) if c.shared_mini else self.flags
        if not self.stage("idempotence", [c.tool, c.out / "rewritten.cpp", "--output",
                                         c.out / "second.cpp", "--report", c.out / "second.json",
                                         "--profile", p.name, "--rules", p.rules, "--", *flags]):
            raise RuntimeError("Second rewrite failed")
        if (c.out / "rewritten.cpp").read_bytes() != (c.out / "second.cpp").read_bytes():
            raise RuntimeError("Rewrite is not idempotent")
        self.summary["idempotence"] = {
            "source_byte_equal": True,
            "second_edit_count": len(json.loads((c.out / "second.json").read_text())["edits"]),
        }

    def frama_checks(self) -> None:
        """观察原始对照，再转换改写输入，要求本轮新 C 非空。

        返回 None，工具缺失或改写转换失败抛 RuntimeError。原始对照退出码
        只记录、不预设；退出 0 不代表没有警告或完整依赖已经被验证。
        """
        c = self.config
        flags = mini_arguments(self.frama_flags, c.conversion_include) if c.shared_mini else self.frama_flags
        if not c.frama_c.is_file():
            raise RuntimeError(f"Missing Frama-C executable: {c.frama_c}")
        if not self.stage("conversion_clang_version", [c.frama_llvm / "bin/clang++", "--version"], conversion=True):
            raise RuntimeError("Cannot determine the conversion toolchain version")
        if not self.stage("conversion_translator_version", [c.converter, "--version"], conversion=True):
            raise RuntimeError("Cannot determine the C++ translator version")
        verify_converter_version((c.out / "logs/conversion_translator_version.log").read_text(), c.frama_llvm)
        if not self.stage("frama_version", [c.frama_c, "-version"], conversion=True):
            raise RuntimeError("Cannot determine Frama-C version")
        self.stage("frama_baseline_mini", frama_command(c, flags, "baseline_mini.cpp", "baseline.c"), conversion=True)
        if not self.stage("frama_rewritten", frama_command(c, flags, "rewritten.cpp", "converted.c"), conversion=True):
            log = (c.out / "logs/frama_rewritten.log").read_text()
            if c.shared_mini and "CXXInheritedCtorInitExpr" in log:
                self.summary["conversion_blockers"] = [
                    "Frama-Clang 0.0.20 (Clang 18.1.8) encountered CXXInheritedCtorInitExpr; identify its owning type before considering the optional source constructor rule or a header adapter"
                ]
            raise RuntimeError("Frama-Clang conversion failed")
        converted = c.out / "converted.c"
        if not converted.is_file() or not converted.stat().st_size:
            raise RuntimeError("Frama-Clang did not produce nonempty C")
        if not self.stage("frama_c_readback", [c.frama_c, converted, "-machdep", "gcc_x86_64"], conversion=True):
            raise RuntimeError("Generated C failed independent Frama-C readback")

    def finish(self) -> None:
        """核对原件未变，收集产物哈希，设置状态并保存。

        返回 None；原件变化抛 RuntimeError，文件读写失败抛 OSError。
        skip_frama 的结果只能是 native_checked，不能冒充 converted。
        """
        if any(digest(path) != value for path, value in self.originals.items()):
            raise RuntimeError("An original input changed during the run")
        self.summary["status"] = "native_checked" if self.config.skip_frama else "converted"
        self.record_inventory()
        compact_verified_artifacts(self.config.out, self.summary)
        self.summary["artifacts_sha256"] = {
            str(path.relative_to(self.config.out)): digest(path) for path in self.config.out.rglob("*")
            if path.is_file() and path.name != "summary.json"
        }
        self.save()

    def run(self) -> int:
        """唯一阶段顺序入口，返回 0=要求阶段均通过，1=失败并保存已有证据。

        native_checked / converted 代表不同验证范围，不合并成“验证正确”。
        """
        try:
            self.save()
            self.stage("clang_version", [self.config.llvm / "bin/clang++", "--version"])
            self.rewrite()
            self.extract_interfaces()
            self.native_checks()
            self.rewrite_headers()
            self.idempotence_check()
            if not self.config.skip_frama:
                self.frama_checks()
            self.finish()
            print(f"{self.summary['status']}: {self.config.out}")
            return 0
        except (RuntimeError, OSError, ValueError) as error:
            self.summary["status"] = "conversion_blocked" if self.summary.get("conversion_blockers") else "failed"
            self.summary["error"] = str(error)
            self.record_inventory()
            self.save()
            evidence = "" if self.config.replace_output else f"\nEvidence: {self.config.out}"
            print(f"Failed: {error}{evidence}", file=sys.stderr)
            return 1
