"""实验矩阵：成功、失败及错误接受都保留证据，不修改真实头文件。"""

from pathlib import Path
import shlex

from environment import FRAMA, MATMUL, IME, MINI, ROOT, REPO, Commands, arguments, save
from emission import logical_models, array_models


def case(out: Path, name: str, header: str, body: str) -> Path:
    """生成独立调用端；header 来自真实路径或本轮自动抽取，body 为人工测试输入。"""
    target = out / 'cases' / (name + '.cpp')
    target.write_text(f'#include "{header}"\n' + body)
    return target


def constructor_signature(probe: dict) -> list[dict]:
    """返回 ArrayRef<int> 的实际构造绑定，用于比较而非只检查解析退出码。"""
    return [{'signature': c['signature'], 'name': c['selected']['name'],
             'pattern': c.get('template_pattern', {}).get('text')}
            for c in probe['calls'] if c['kind'] == 'constructor'
            and c.get('selected', {}).get('name', '').startswith('llvm::ArrayRef')]


def run_library(out: Path, commands: Commands) -> dict:
    """运行真实 LLVM 接口抽取与逐项消融；返回模型元数据和实际绑定。"""
    probe = commands.observe('library_real', ROOT / 'fixtures/library_input.cpp')
    models = out / 'extracted'
    metadata = {'logical': logical_models(probe, models), 'array': array_models(probe, models)}
    save(out / 'extraction_manifest.json', metadata)
    logical_body = r'''
int main() {
  auto failed = llvm::failure();
  auto succeeded = llvm::failure(false);
  return !(llvm::failed(failed) && !llvm::failed(succeeded));
}
'''
    logical_inputs = {}
    for mode, header in [('real', 'llvm/Support/LogicalResult.h'),
                         ('surface', str(models / 'logical_surface_bodies.h')),
                         ('closure', str(models / 'logical_closure_bodies.h')),
                         ('declarations', str(models / 'logical_declarations.h'))]:
        source = case(out, 'logical_' + mode, header, logical_body)
        logical_inputs[mode] = source
        commands.native('logical_' + mode + '_syntax', source,
                        'failure' if mode == 'surface' else 'success')
        if mode != 'surface':
            executable = out / 'executables' / ('logical_' + mode)
            result = commands.link('logical_' + mode + '_link', source, executable,
                                   'failure' if mode == 'declarations' else 'success')
            if result['exit_code'] == 0:
                commands.run('logical_' + mode + '_execute', [executable])
            trait = case(out, 'logical_' + mode + '_trait', header,
                         '#include <type_traits>\n'
                         'static_assert(!std::is_empty_v<llvm::LogicalResult>);\n')
            commands.native('logical_' + mode + '_trait', trait,
                            'failure' if mode == 'declarations' else 'success')
    # 只转换无标准库依赖的自动抽取模型；不把 C 输出当形式化证明。
    commands.run('frama_version', [FRAMA, '-version'])
    for mode in ['closure', 'declarations']:
        commands.run('frama_logical_' + mode, [FRAMA, logical_inputs[mode], '-cxx-nostdinc',
                     '-machdep', 'gcc_x86_64',
                     f'-cxx-clang-command={FRAMA.parent / "framaCIRGen"}',
                     '-fclang-cpp-extra-args=' + shlex.join(commands.flags),
                     '-cxx-parseable-output', '-print', '-ocode',
                     out / 'converted' / ('logical_' + mode + '.c')], 'observe')
    array_runtime = r'''
int main() {
  int values[] = {2, 5, 9};
  llvm::ArrayRef<int> range(values, 3);
  return !(range.size() == 3 && range[1] == 5 && range.data() == values);
}
'''
    sources = {}
    for mode, header in [('real', 'llvm/ADT/ArrayRef.h'),
                         ('raw', str(models / 'array_raw.h')),
                         ('closed', str(models / 'array_closed.h')),
                         ('no_fields', str(models / 'array_no_fields.h')),
                         ('declarations', str(models / 'array_declarations.h'))]:
        source = case(out, 'array_' + mode, header, array_runtime)
        sources[mode] = source
        commands.native('array_' + mode + '_syntax', source,
                        'failure' if mode in {'raw', 'no_fields'} else 'success')
        if mode in {'real', 'closed'}:
            executable = out / 'executables' / ('array_' + mode)
            r = commands.link('array_' + mode + '_link', source, executable,
                              'failure' if mode == 'real' else 'success')
            if mode == 'real':
                # 第一轮实测出现 ABI 检查符号；保留不链接库的失败，另做配套库对照。
                support = REPO.parent / 'build-v0.0.4/llvm/lib/libLLVMSupport.a'
                r = commands.link('array_real_link_support', source, executable, extra=[support])
            if r['exit_code'] == 0:
                commands.run('array_' + mode + '_execute', [executable])
    constant = ('constexpr int data[] = {1, 2};\n'
                'constexpr llvm::ArrayRef<int> ref(data, 2);\n')
    for mode, header in [('real', 'llvm/ADT/ArrayRef.h'),
                         ('closed', str(models / 'array_closed.h')),
                         ('declarations', str(models / 'array_declarations.h'))]:
        source = case(out, 'array_' + mode + '_constexpr', header, constant)
        commands.native('array_' + mode + '_constexpr', source,
                        'failure' if mode == 'declarations' else 'success')
    scalar = ('struct ScalarLike { operator const int&() const; };\n'
              'void accept(const ScalarLike &x) { llvm::ArrayRef<int> ref(x); }\n')
    invalid = ('struct Unrelated {};\n'
               'void reject(const Unrelated &x) { llvm::ArrayRef<int> ref(x); }\n')
    bindings = {}
    for mode, header in [('real', 'llvm/ADT/ArrayRef.h'),
                         ('constrained', str(models / 'array_declarations.h')),
                         ('unconstrained', str(models / 'array_unconstrained_declarations.h'))]:
        source = case(out, 'array_' + mode + '_scalar', header, scalar)
        ast = commands.observe('array_' + mode + '_binding', source)
        bindings[mode] = constructor_signature(ast)
        negative = case(out, 'array_' + mode + '_negative', header, invalid)
        commands.native('array_' + mode + '_negative', negative,
                        'success' if mode == 'unconstrained' else 'failure')
    save(out / 'array_bindings.json', bindings)
    return {'models': metadata, 'array_bindings': bindings}


def binding_key(call: dict):
    """保留位置/种类/绑定/类型/默认实参；相同位置可能有多个隐式构造，按多重集比较。"""
    return (call['line'], call['column'], call['kind'],
            call.get('selected', {}).get('name'), call.get('signature'),
            call['result_type'], call['argument_count'], call['default_arguments'])


def run_pass_inventory(out: Path, commands: Commands) -> dict:
    """完整 Matmul 与 LowerIME 真实 AST 观察，及 Matmul 同文 include 替换对照。

    使用既有人工 mini 作为观察对照，明确不是本实验自动抽出的完整 pass 头。
    保留行数使同一调用位置可比较；宏展开另有记录，不以文本通过代替绑定核对。
    """
    from collections import Counter
    real = commands.observe('matmul_real', MATMUL)
    ime_flags, _ = arguments(IME, llvm=commands.llvm, system_prefix=commands.system_prefix)
    ime = commands.observe('ime_real', IME, ime_flags)
    lines = MATMUL.read_text().splitlines(keepends=True)
    includes = real['includes']
    inserted = False
    for inc in includes:
        index = inc['line'] - 1
        # 本轮原文件只有单行 include；不推广为任意预处理指令重写器。
        if not lines[index].lstrip().startswith('#include'):
            raise ValueError('Unexpected include layout')
        lines[index] = f'#include "{MINI}"\n' if not inserted else '\n'
        inserted = True
    source = out / 'cases' / 'matmul_existing_mini.cpp'
    source.write_text(''.join(lines))
    mini = commands.observe('matmul_existing_mini', source)
    before, after = Counter(map(binding_key, real['calls'])), Counter(map(binding_key, mini['calls']))
    removed = list((before - after).elements())
    added = list((after - before).elements())
    report = {'matmul': {'real_calls': len(real['calls']), 'mini_calls': len(mini['calls']),
                         'exact_entries_in_common': sum((before & after).values()),
                         'real_only': removed, 'mini_only': added,
                         'real_types': len(real['types']), 'mini_types': len(mini['types']),
                         'real_override_edges': len(real['overrides']),
                         'mini_override_edges': len(mini['overrides']),
                         'default_argument_calls_real': sum(c['default_arguments'] > 0 for c in real['calls']),
                         'template_pattern_calls_real': sum('template_pattern' in c for c in real['calls'])},
              'lower_ime': {'calls': len(ime['calls']), 'types': len(ime['types']),
                            'override_edges': len(ime['overrides']),
                            'macro_names': sorted({m['name'] for m in ime['macros'] if m['main_file']})},
              'limits': ['Canonical signature equality is a diagnostic, not semantic equivalence',
                         'Differences may include intentional aliases/inheritance normalization',
                         'No transitive implementation closure or indirect-call target analysis',
                         'Existing complete mini is a manual reference, not pilot extraction output']}
    save(out / 'pass_comparison.json', report)
    return report
