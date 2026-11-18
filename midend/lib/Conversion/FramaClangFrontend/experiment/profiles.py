"""集中维护已审计 pass 的输入、参考和验收范围，供命令行入口选择。"""

from dataclasses import dataclass
from pathlib import Path
from typing import Mapping

FRONTEND_ROOT = Path(__file__).resolve().parents[1]
REPO = FRONTEND_ROOT.parents[3]
WORKSPACE = REPO.parent
GRAPH = FRONTEND_ROOT.parent / "GraphRedundancyElimination"
SYSTEM_PREFIX = Path("/home/ce/Desktop/framaclang/environment/llvm18")
FRONTEND_LLVM = SYSTEM_PREFIX
# 默认解析、改写、提取和转换均使用 18；两个阶段仍可独立指定工具链。
# 23 的源码、配置和产物已备份，原 build-llvm23/ 保留为历史实验。
CONVERSION_LLVM = SYSTEM_PREFIX
CONVERSION_CIRGEN = Path.home() / ".opam/framac-study/bin/framaCIRGen"
FRONTEND_TOOL = FRONTEND_ROOT / "build-llvm18/buddy-framac-rewrite"
MINI_ROOT = FRONTEND_ROOT.parent / "MiniMLIR"


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
    reference: Path
    rules: str
    expected: Mapping[str, int]
    require_reference_match: bool = False
    compatibility_notes: tuple[str, ...] = ()
    reference_source: Path | None = None  # Audited updated source; historical inputs stay immutable.
    rounds: tuple[str, ...] = ()  # Fresh AST between rules with enclosing replacements.
    reference_real_includes: bool = False  # Imported references keep MLIR include paths.


# 新增 pass 先登记自己的证据；不能只放宽 expected 以绕过失败。
PASS_PROFILES = {
    "reshape": PassProfile(
        "reshape", GRAPH / "SimplifyTosaReshape.cpp",
        GRAPH / "test_framaclang/simplify_tosa_reshape/full_pass_v2/final",
        "namespace,while,capture,guard",
        {"interface-substitution": 5, "namespace": 1, "namespace-reference": 1,
         "while": 1, "capture-pointers": 2, "capture": 2, "guard": 1},
    ),
    "matmul-scalar": PassProfile(
        "matmul-scalar", GRAPH / "SimplifyTosaMatmulScalar.cpp",
        GRAPH / ("test_framaclang/simplify_tosa_matmul_scalar/full_pass/"
                 "steps/100_complete_pass"),
        "namespace,guard",
        {"interface-substitution": 8, "namespace": 1,
         "namespace-reference": 1, "guard": 6},
        require_reference_match=True,
        reference_source=FRONTEND_ROOT / "tests/references/matmul-scalar-guard-binding.cpp",
        compatibility_notes=(
            "guard: brace-only scalar conditions lose their block in generated C; bind the original condition to a fresh block-local bool without splitting ||",
        ),
    ),
    "lower-ime": PassProfile(
        "lower-ime", FRONTEND_ROOT.parent / "LowerIME/LowerIMEPass.cpp",
        FRONTEND_ROOT.parent / "LowerIME/test_framaclang/steps/072_complete_pass",
        "namespace",
        {"interface-substitution": 10, "namespace": 1, "namespace-reference": 2},
    ),
    "silu-fusion": PassProfile(
        "silu-fusion", FRONTEND_ROOT.parent / "TosaElementwiseFusion/SiLUFusion.cpp",
        FRONTEND_ROOT.parent / ("TosaElementwiseFusion/test_framaclang/steps/"
                                "104_arrayref_complete_pass"),
        "namespace,if-condition,guard,value-range,capture",
        {"interface-substitution": 11, "namespace": 2, "namespace-reference": 1,
         "if-condition": 1, "guard": 1, "value-range-preamble": 1,
         "value-range": 2, "capture-pointers": 1, "capture": 1},
        compatibility_notes=(
            "capture: original rule rejected declaration initializers; SiLU alone accepts a single automatic local declaration",
            "guard: original rule required a direct return; SiLU alone accepts a block containing exactly one failure return",
            "plan: independently generated preludes share an offset; explicit insertion orders 10/20 compose them, other overlaps still reject",
        ),
    ),
}


# four_pass(1).zip is the user's authoritative LLVM 18 reference. Its immutable
# source/header snapshot remains distinct from the generated shared model.
_IMPORTED = FRONTEND_ROOT / "tests/references/four_pass_clang18"
for _name, _folder, _source, _rules in (
    ("assume-tight", "AssumeTightMemRefLayout", "AssumeTightMemRefLayout/AssumeTightMemRefLayout.cpp", ("namespace", "capture", "range-for", "if-condition")),
    ("eliminate-copy", "EliminateMemRefCopy", "EliminateMemRefCopy/EliminateMemRefCopy.cpp", ("namespace", "constructor", "range-for", "if-condition", "guard")),
    ("gpu-memcpy", "ConvertMemcpyToGPU", "MLIRGPU/ConvertMemcpyToGPU.cpp", ("namespace", "capture", "range-for", "if-condition")),
    ("depthwise-conv", "DepthwiseConvNhwcHwc", "DepthwiseConvOptimization/DepthwiseConvNhwcHwc.cpp", ("namespace", "const-local", "init-list", "capture")),
):
    PASS_PROFILES[_name] = PassProfile(
        _name, FRONTEND_ROOT.parent / _source, _IMPORTED / _folder,
        ",".join(_rules), {
            "assume-tight": {"namespace": 1, "namespace-reference": 1, "capture-pointers": 1, "capture": 1, "range-for": 3, "if-condition": 1},
            "eliminate-copy": {"namespace": 1, "namespace-reference": 1, "range-for": 5, "if-condition": 6, "guard": 1, "constructor": 1},
            "gpu-memcpy": {"namespace": 1, "namespace-reference": 1, "capture-pointers": 1, "capture": 1, "range-for": 2, "if-condition": 5},
            "depthwise-conv": {"namespace": 2, "namespace-reference": 3, "const-local": 6, "init-list": 24, "init-list-preamble": 23, "capture-pointers": 1, "capture": 1},
        }[_name], reference_source=_IMPORTED / _folder / "rewritten.cpp",
        rounds=_rules, reference_real_includes=True,
        compatibility_notes=("Imported reference differences audited in docs/FOUR_PASS_IMPORT.md",),
    )


PASS_PROFILES["builtin-transpose"] = PassProfile(
    "builtin-transpose", FRONTEND_ROOT.parent / "TransposeOptimization/BuiltinTransposeVectorization.cpp",
    FRONTEND_ROOT / "tests/references/builtin-transpose", "namespace,const-local,init-list,capture",
    {"namespace": 2, "namespace-reference": 3, "const-local": 7, "init-list": 84, "init-list-preamble": 33, "capture-pointers": 2, "capture": 2},
    reference_source=FRONTEND_ROOT / "tests/references/builtin-transpose/rewritten.cpp",
    rounds=("namespace", "const-local", "init-list", "capture"), reference_real_includes=True,
    compatibility_notes=(
        "capture: prior rule rejects mixed handle/scalar/reference-parameter/this captures; bounded borrowed-pointer adapter retains original object and per-instance member identity",
        "reference: archived globalized member and [=] copies are deliberately not adopted; see docs/BUILTIN_TRANSPOSE.md",
    ),
)


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
    database: Path
    frama_c: Path
    out: Path
    skip_frama: bool = False
    frama_llvm: Path = CONVERSION_LLVM
    system_prefix: Path = SYSTEM_PREFIX
    frama_cirgen: Path | None = CONVERSION_CIRGEN
    replace_output: bool = False

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
