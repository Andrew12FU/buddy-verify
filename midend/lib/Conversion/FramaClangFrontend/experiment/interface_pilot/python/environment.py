"""接口抽象先驱实验的工具链边界；与改写同属一个前端，实验参数暂分别维护。"""

import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
FRONTEND_ROOT = ROOT.parents[1]
REPO = FRONTEND_ROOT.parents[3]
sys.path.insert(0, str(FRONTEND_ROOT))
from experiment.profiles import (CONVERSION_CIRGEN, CONVERSION_LLVM, FRONTEND_LLVM,
                                 FRONTEND_TOOL, SYSTEM_PREFIX)
from experiment.toolchain import (compiler_arguments, compiler_environment,
                                  conversion_arguments, verify_parser_version)

PROBE = FRONTEND_TOOL.parent / "header-extraction-probe"
LLVM = FRONTEND_LLVM
DATABASE = REPO.parent / 'build-v0.0.4/buddy/compile_commands.json'
FRAMA = Path.home() / '.opam/framac-study/bin/frama-c'
MATMUL = FRONTEND_ROOT.parent / 'GraphRedundancyElimination/SimplifyTosaMatmulScalar.cpp'
IME = FRONTEND_ROOT.parent / 'LowerIME/LowerIMEPass.cpp'
MINI = FRONTEND_ROOT.parent / ('GraphRedundancyElimination/test_framaclang/'
                     'simplify_tosa_matmul_scalar/full_pass/steps/100_complete_pass/mini_mlir.h')


def digest(path: Path) -> str:
    """返回文件字节 SHA-256；用于固定真实输入与抽取器，不代表语义相同。"""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path: Path, value) -> None:
    """将 JSON 保存到本轮目录；目录保护由入口完成。"""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


def arguments(source: Path = MATMUL, *, llvm: Path = LLVM,
              system_prefix: Path = SYSTEM_PREFIX) -> tuple[list[str], Path]:
    """Use the same audited source/system flags as the rewrite runner."""
    flags, cwd, _ = compiler_arguments(DATABASE, source, llvm, system_prefix)
    return flags, cwd


class Commands:
    """只写本轮日志的进程边界；预期失败同样保存，超时与崩溃不算预期语法拒绝。"""
    def __init__(self, out: Path, flags: list[str], cwd: Path, *, llvm: Path = LLVM,
                 probe: Path = PROBE, frama_llvm: Path = CONVERSION_LLVM,
                 system_prefix: Path = SYSTEM_PREFIX, frama_cirgen: Path | None = CONVERSION_CIRGEN):
        self.out, self.flags, self.cwd = out, flags, cwd
        self.llvm, self.probe, self.system_prefix = llvm, probe, system_prefix
        self.frama_llvm = frama_llvm
        self.converter = frama_cirgen or FRAMA.parent / 'framaCIRGen'
        self.results = {}
        self.env = compiler_environment(llvm)
        self.frama_env = compiler_environment(frama_llvm, FRAMA)

    def run(self, name: str, argv, expected: str = 'success') -> dict:
        """执行无 shell argv，记录实际结果与预期；返回记录，非零不自动终止研究。

        expected 为 success、failure 或 observe。failure 只接受正常正退出码，
        不接受信号退出/超时。observe 只记录，不被计作验收通过。
        """
        argv = [str(a) for a in argv]
        conversion = Path(argv[0]).resolve() == FRAMA.resolve()
        if conversion:
            argv = ['-fclang-cpp-extra-args=' + shlex.join(conversion_arguments(
                        shlex.split(arg.split('=', 1)[1]), self.frama_llvm))
                    if arg.startswith('-fclang-cpp-extra-args=') else
                    '-cxx-clang-command=' + str(self.converter)
                    if arg.startswith('-cxx-clang-command=') else arg for arg in argv]
        try:
            r = subprocess.run(argv, cwd=self.cwd, env=self.frama_env if conversion else self.env,
                               capture_output=True, text=True, timeout=180)
            code, stdout, stderr = r.returncode, r.stdout, r.stderr
        except subprocess.TimeoutExpired as error:
            code, stdout, stderr = 124, '', f'Timeout: {error}'
        self.out.joinpath('logs', name + '.log').write_text(
            f'$ {shlex.join(argv)}\ncwd: {self.cwd}\nexit_code: {code}\n'
            f'--- stdout ---\n{stdout}\n--- stderr ---\n{stderr}')
        matched = (code == 0 if expected == 'success' else
                   0 < code < 124 if expected == 'failure' else None)
        record = {'command': argv, 'cwd': str(self.cwd), 'exit_code': code,
                  'expected': expected, 'matched': matched, 'log': 'logs/' + name + '.log'}
        self.results[name] = record
        save(self.out / 'commands.json', self.results)
        return record

    def native(self, name: str, source: Path, expected='success', flags=None):
        """只做原生语法/语义检查，不能替代链接、执行或调用绑定比较。"""
        return self.run(name, [self.llvm / 'bin/clang++', *(flags or self.flags),
                               '-fsyntax-only', source], expected)

    def link(self, name: str, source: Path, output: Path, expected='success', extra=()):
        """Select the Conda GCC runtime explicitly for the relocated Ubuntu driver."""
        roots = list((self.system_prefix / 'lib/gcc/x86_64-conda-linux-gnu').glob('*/crtbeginS.o'))
        if len(roots) != 1:
            raise ValueError('Expected one audited GCC runtime directory')
        return self.run(name, [self.llvm / 'bin/clang++', *self.flags,
                              f'--gcc-install-dir={roots[0].parent}', source, *extra,
                              '-o', output], expected)

    def observe(self, name: str, source: Path, flags=None):
        """用独立 C++ AST 探针解析，返回新建的报告；解析失败由调用方检查。"""
        report = self.out / 'observations' / (name + '.json')
        result = self.run(name, [self.probe, source,
                                 '--report', report, '--', *(flags or self.flags)])
        if result['exit_code']:
            raise RuntimeError(f'Probe failed: {name}')
        observation = json.loads(report.read_text())
        verify_parser_version(observation, self.llvm)
        return observation
