"""集中维护已审计 pass 的输入、参考和验收范围，供命令行入口选择。"""

from dataclasses import dataclass
from pathlib import Path
from typing import Mapping

from .paths import (FRONTEND_ROOT, REPO, WORKSPACE, MINI_ROOT, SYSTEM_PREFIX,
                    FRONTEND_LLVM, CONVERSION_LLVM, CONVERSION_CIRGEN, FRONTEND_TOOL)


@dataclass(frozen=True)
class PassProfile:
    """一个 pass 的审计配置，不包含 AST 算法。

    Attributes:
        name: 与 C++ --profile 对应的标识。
        source: 只读原始 Buddy 源码。
        reference: 含人工 input.cpp 和 mini_mlir.h 的历史参考目录。
        rules: 传给 C++ 的逗号分隔规则名。
        expected: 已审阅过的编辑数；变动时停止，避免静默遗漏。
        require_reference_match: 是否要求自动 C++ 与人工参考逐字节一致。
            Reshape 的 while/capture 表达方式有已知差别，因此不强制。
        compatibility_notes: 旧规则复用时发现的不适配和本轮受限扩展，随摘要保留。
        reference_source: 可选的新版审计源码参考；不覆盖 reference 内的历史快照。
    """

    name: str
    source: Path
    reference: Path | None
    rules: str
    expected: Mapping[str, int] | None
    require_reference_match: bool = False
    compatibility_notes: tuple[str, ...] = ()
    reference_source: Path | None = None  # Audited updated source; historical inputs stay immutable.
    rounds: tuple[str, ...] = ()  # Fresh AST between rules with enclosing replacements.
    namespace_prefix: str = ""
    reference_real_includes: bool = False  # Imported references keep MLIR include paths.

    @property
    def ast_rounds(self) -> tuple[str, ...]:
        """Rules owning nested source ranges use fresh ASTs independently of the pass."""
        if self.rounds:
            return self.rounds
        rules = tuple(self.rules.split(","))
        return rules if {"if-condition", "guard", "init-list"}.intersection(rules) else ()


# Audit profiles select rules and repository-relative input/reference data.
PASS_PROFILES = {
    'reshape': PassProfile(
        name='reshape', source=REPO / '02_passes/reshape/original.cpp',
        reference=REPO / '03_manual_passes/buddy_four/reshape', rules='namespace,while,capture,guard',
        expected={'interface-substitution': 5, 'namespace': 1, 'namespace-reference': 1, 'while': 1, 'capture-pointers': 2, 'capture': 2, 'guard': 1}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/buddy_four/reshape/rewritten.cpp', reference_real_includes=False,
        namespace_prefix='SimplifyTosaReshape', rounds=(),
        compatibility_notes=()),
    'matmul-scalar': PassProfile(
        name='matmul-scalar', source=REPO / '02_passes/matmul-scalar/original.cpp',
        reference=REPO / '03_manual_passes/buddy_four/matmul-scalar', rules='namespace,guard',
        expected={'interface-substitution': 8, 'namespace': 1, 'namespace-reference': 1, 'guard': 7}, require_reference_match=True,
        reference_source=FRONTEND_ROOT / "tests/references/matmul-scalar-structural-rules.cpp", reference_real_includes=False,
        namespace_prefix='SimplifyTosaMatmulScalar', rounds=(),
        compatibility_notes=('guard: match every runtime if + built-in ||, including the loop check; preserve both branches and bind the complete condition once',)),
    'lower-ime': PassProfile(
        name='lower-ime', source=REPO / '02_passes/lower-ime/original.cpp',
        reference=REPO / '03_manual_passes/buddy_four/lower-ime', rules='namespace',
        expected={'interface-substitution': 10, 'namespace': 1, 'namespace-reference': 2}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/buddy_four/lower-ime/rewritten.cpp', reference_real_includes=False,
        namespace_prefix='LowerIMEPass', rounds=(),
        compatibility_notes=()),
    'silu-fusion': PassProfile(
        name='silu-fusion', source=REPO / '02_passes/silu-fusion/original.cpp',
        reference=REPO / '03_manual_passes/buddy_four/silu-fusion', rules='namespace,if-condition,guard,value-range,capture',
        expected={'interface-substitution': 11, 'namespace': 2, 'namespace-reference': 1, 'if-condition': 1, 'guard': 1, 'value-range-preamble': 1, 'value-range': 2, 'capture-pointers': 1, 'capture': 1}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/buddy_four/silu-fusion/rewritten.cpp', reference_real_includes=False,
        namespace_prefix='SiLUFusion', rounds=(),
        compatibility_notes=('capture: all profiles use structural capture-tree planning, including single automatic local declarations', 'guard: the same structural rule preserves arbitrary then/else bodies in every profile', 'plan: independently generated preludes share an offset; explicit insertion orders 10/20 compose them, other overlaps still reject')),
    'assume-tight': PassProfile(
        name='assume-tight', source=REPO / '02_passes/assume-tight/original.cpp',
        reference=REPO / '03_manual_passes/four_pass_clang18/AssumeTightMemRefLayout', rules='namespace,capture,range-for,if-condition',
        expected={'namespace': 1, 'namespace-reference': 1, 'capture-pointers': 1, 'capture': 1, 'range-for': 3, 'if-condition-normalize': 1, 'if-condition': 1}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/four_pass_clang18/AssumeTightMemRefLayout/rewritten.cpp', reference_real_includes=True,
        namespace_prefix='AssumeTightMemRefLayout', rounds=('namespace', 'capture', 'range-for', 'if-condition'),
        compatibility_notes=('Imported reference differences audited in docs/FOUR_PASS_IMPORT.md',)),
    # constructor is an opt-in compatibility rule; none of the nine defaults uses it.
    # See docs/OPTIONAL_CONSTRUCTOR_RULE.md for the latent converter limitation.
    'eliminate-copy': PassProfile(
        name='eliminate-copy', source=REPO / '02_passes/eliminate-copy/original.cpp',
        reference=REPO / '03_manual_passes/four_pass_clang18/EliminateMemRefCopy', rules='namespace,range-for,if-condition,guard',
        expected={'namespace': 1, 'namespace-reference': 1, 'range-for': 5, 'if-condition-normalize': 3, 'if-condition': 6, 'guard': 2}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/four_pass_clang18/EliminateMemRefCopy/rewritten.cpp', reference_real_includes=True,
        namespace_prefix='EliminateMemRefCopy', rounds=('namespace', 'range-for', 'if-condition', 'guard'),
        compatibility_notes=('Imported reference differences audited in docs/FOUR_PASS_IMPORT.md',)),
    'gpu-memcpy': PassProfile(
        name='gpu-memcpy', source=REPO / '02_passes/gpu-memcpy/original.cpp',
        reference=REPO / '03_manual_passes/four_pass_clang18/ConvertMemcpyToGPU', rules='namespace,capture,range-for,if-condition',
        expected={'namespace': 1, 'namespace-reference': 1, 'capture-pointers': 1, 'capture': 1, 'range-for': 2, 'if-condition-normalize': 1, 'if-condition': 5}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/four_pass_clang18/ConvertMemcpyToGPU/rewritten.cpp', reference_real_includes=True,
        namespace_prefix='ConvertMemcpyToGPU', rounds=('namespace', 'capture', 'range-for', 'if-condition'),
        compatibility_notes=('Imported reference differences audited in docs/FOUR_PASS_IMPORT.md',)),
    'depthwise-conv': PassProfile(
        name='depthwise-conv', source=REPO / '02_passes/depthwise-conv/original.cpp',
        reference=REPO / '03_manual_passes/four_pass_clang18/DepthwiseConvNhwcHwc', rules='namespace,const-local,init-list,capture',
        expected={'namespace': 2, 'namespace-reference': 3, 'const-local': 6, 'init-list': 24, 'init-list-preamble': 23, 'capture-pointers': 1, 'capture': 1}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/four_pass_clang18/DepthwiseConvNhwcHwc/rewritten.cpp', reference_real_includes=True,
        namespace_prefix='DepthwiseConvNhwcHwc', rounds=('namespace', 'const-local', 'init-list', 'capture'),
        compatibility_notes=('Imported reference differences audited in docs/FOUR_PASS_IMPORT.md',)),
    'builtin-transpose': PassProfile(
        name='builtin-transpose', source=REPO / '02_passes/builtin-transpose/original.cpp',
        reference=REPO / '03_manual_passes/builtin-transpose', rules='namespace,const-local,init-list,capture',
        expected={'namespace': 2, 'namespace-reference': 3, 'const-local': 7, 'init-list': 84, 'init-list-preamble': 33, 'capture-pointers': 2, 'capture': 2}, require_reference_match=False,
        reference_source=REPO / '03_manual_passes/builtin-transpose/rewritten.cpp', reference_real_includes=True,
        namespace_prefix='BuiltinTransposeVectorization', rounds=('namespace', 'const-local', 'init-list', 'capture'),
        compatibility_notes=('capture: generic capture-tree planner retains reference referents, value snapshots and per-instance integer field identity', 'reference: archived globalized member and [=] copies are deliberately not adopted; see docs/BUILTIN_TRANSPOSE.md')),
}


@dataclass(frozen=True)
class RunConfig:
    """单轮执行的路径和开关；由入口完成选择，各阶段只读使用。

    所有产物只能写入 out；replace_output 表示固定 current 目录的事务式更新。
    tool/llvm/frama_c 为已安装工具路径；database
    为真实编译数据库。frama_llvm 为转换器配套环境；system_prefix 单独
    固定 sysroot/GCC 标准库。skip_frama=True 时最终状态只能是 native_checked。
    配置冻结只禁止重新赋值，不宣称文件内容在运行中自动不可变。
    """

    profile: PassProfile
    llvm: Path
    tool: Path
    interface: Path | None
    database: Path | None
    frama_c: Path
    out: Path
    skip_frama: bool = False
    frama_llvm: Path = CONVERSION_LLVM
    system_prefix: Path | None = SYSTEM_PREFIX
    frama_cirgen: Path | None = CONVERSION_CIRGEN
    replace_output: bool = False
    parse_mode: str = "model"
    compile_source: Path | None = None
    extra_arguments: tuple[str, ...] = ()

    @property
    def shared_mini(self) -> bool:
        """None selects canonical MiniMLIR extraction; a file selects legacy mode."""
        return self.interface is None

    @property
    def slicer(self) -> Path:
        """Use the slicer built beside the selected rewrite tool (same Clang ABI)."""
        return self.tool.parent / "buddy-mini-slice"

    @property
    def mini_include(self) -> Path:
        """Per-run generated headers, never a separately maintained pass model."""
        return self.out / "include"

    @property
    def conversion_include(self) -> Path:
        """独立的头文件兼容改写结果，原始提取库继续用于接口检查。"""
        return self.out / "conversion-include"

    @property
    def converter(self) -> Path:
        """External C++ translator; independent from the Frama-C OCaml executable."""
        return self.frama_cirgen or self.frama_c.parent / "framaCIRGen"

    @property
    def source(self) -> Path:
        """返回原始主文件的绝对真实路径；不创建或修改文件。"""
        return self.profile.source.resolve()

    @property
    def reference(self) -> Path:
        """返回人工参考目录；其中的输入和 mini 始终只读。"""
        return self.profile.reference
