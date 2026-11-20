#!/usr/bin/env python3
"""运行接口抽象先驱实验；默认清理临时产物，显式输出路径才保留诊断。"""
import argparse
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent / 'python'))
from environment import ROOT, FRONTEND_ROOT, PROBE, REPO, LLVM, MATMUL, IME, MINI, Commands, arguments, digest, save
from experiments import run_library, run_pass_inventory
from experiment.profiles import CONVERSION_CIRGEN, CONVERSION_LLVM, SYSTEM_PREFIX


def run(argv: list[str]) -> int:
    """返回0表示实验预期全部满足；包括预期失败，不代表生成了完整 MLIR mini。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--llvm-prefix', type=Path, default=LLVM)
    parser.add_argument('--frama-llvm-prefix', type=Path, default=CONVERSION_LLVM)
    parser.add_argument('--frama-cirgen', type=Path, default=CONVERSION_CIRGEN)
    parser.add_argument('--system-prefix', type=Path, default=SYSTEM_PREFIX)
    parser.add_argument('--probe', type=Path, default=PROBE)
    args = parser.parse_args(argv)
    out = args.output_dir.resolve()
    if out.exists():
        parser.error('Use a new output directory; prior evidence is immutable')
    for name in ['logs', 'observations', 'extracted', 'cases', 'executables', 'converted']:
        (out / name).mkdir(parents=True)
    flags, cwd = arguments(llvm=args.llvm_prefix, system_prefix=args.system_prefix)
    commands = Commands(out, flags, cwd, llvm=args.llvm_prefix, probe=args.probe,
                        frama_llvm=args.frama_llvm_prefix, system_prefix=args.system_prefix,
                        frama_cirgen=args.frama_cirgen)
    paths = [MATMUL, IME, MINI,
             REPO / 'llvm/llvm/include/llvm/Support/LogicalResult.h',
             REPO / 'llvm/llvm/include/llvm/ADT/ArrayRef.h']
    original = {str(p): digest(p) for p in paths}
    source_files = [FRONTEND_ROOT / 'CMakeLists.txt', ROOT / 'run.py',
                    *FRONTEND_ROOT.glob('include/interface/*.h'),
                    *FRONTEND_ROOT.glob('src/interface/*.cpp'),
                    *ROOT.glob('python/*.py'), *ROOT.glob('fixtures/*.cpp')]
    summary = {'schema_version': 1, 'status': 'in_progress', 'out': str(out),
               'original_sha256': original, 'effective_flags': flags,
               'pilot_sources_sha256': {str(p.relative_to(FRONTEND_ROOT)): digest(p) for p in sorted(source_files)},
               'probe_binary_sha256': digest(args.probe),
               'toolchains': {'frontend': str(args.llvm_prefix), 'conversion': str(args.frama_llvm_prefix),
                              'system_headers': str(args.system_prefix), 'converter': str(commands.converter)},
               'limits': ['Bounded pilot, not an automatic whole-pass MLIR header generator',
                          'No ACSL inference, WP/Eva execution or formal proof',
                          'Selected source inputs pinned; not a hermetic snapshot of all system headers']}
    save(out / 'summary.json', summary)
    try:
        commands.run('clang_version', [args.llvm_prefix / 'bin/clang++', '--version'])
        summary['library'] = run_library(out, commands)
        summary['passes'] = run_pass_inventory(out, commands)
        if any(digest(Path(p)) != sha for p, sha in original.items()):
            raise RuntimeError('An observed original input changed')
        summary['originals_unchanged'] = True
        bad = [name for name, r in commands.results.items() if r['matched'] is False]
        summary['unexpected_results'] = bad
        bindings = summary['library']['array_bindings']
        signatures = {k: [v['signature'] for v in values] for k, values in bindings.items()}
        summary['binding_checks'] = {
            'constrained_matches_real': signatures['constrained'] == signatures['real'],
            'constraint_erasure_changes_binding': signatures['unconstrained'] != signatures['real'],
            'all_three_have_one_constructor': all(len(v) == 1 for v in signatures.values())}
        accepted = not bad and all(summary['binding_checks'].values())
        summary['status'] = 'pilot_completed' if accepted else 'unexpected_results'
        return_code = 0 if accepted else 1
    except Exception as error:
        summary['status'], summary['error'] = 'failed', f'{type(error).__name__}: {error}'
        return_code = 1
    save(out / 'summary.json', summary)
    print(f'{summary["status"]}: {out}')
    if return_code:
        print(summary.get('error') or summary.get('unexpected_results'))
    return return_code


def main() -> int:
    """Run without accumulating archives unless a diagnostic path was requested."""
    argv = sys.argv[1:]
    if any(arg == '--output-dir' or arg.startswith('--output-dir=') for arg in argv):
        return run(argv)
    with tempfile.TemporaryDirectory(prefix='buddy-interface-pilot-') as temporary:
        result = run([*argv, '--output-dir', str(Path(temporary) / 'run')])
    print('Interface pilot temporary output cleaned; use --output-dir to retain diagnostics.')
    return result


if __name__ == '__main__':
    raise SystemExit(main())
