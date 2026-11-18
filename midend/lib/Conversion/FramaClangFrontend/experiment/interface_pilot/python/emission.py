"""有界模型生成：消费真实 AST 切片，不用正则表达式理解 C++ 声明。"""

from pathlib import Path


def declaration(item: dict, bodies: bool) -> str:
    """返回一条真实声明/定义；仅声明策略沿用 C++ 端 AST 截取的声明范围。"""
    if bodies or 'declaration_text' not in item:
        return item['text'].rstrip(';') + ';'
    return item['declaration_text']


def logical_models(probe: dict, out: Path) -> dict:
    """从两个真实 free function 向短方法、构造及字段引用求有限闭包。

    返回成员数/闭包边等证据；只支持当前无基类 LogicalResult，未知边直接失败。
    不推导库行为规约，不把只用到的成员视为完整 C++ 类型性质。
    """
    library = probe['library']
    members = library['LogicalResult']['members']
    roots = [library['failure'], library['failed']]
    available = {m['id']: m for m in [*members, *roots]}
    pending, visited = [r['id'] for r in roots], set()
    while pending:
        key = pending.pop()
        if key in visited:
            continue
        if key not in available:
            raise ValueError(f'Unresolved LogicalResult edge: {key}')
        visited.add(key)
        pending.extend(available[key].get('dependencies', []))
    chosen = [m for m in members if m['id'] in visited]
    direct_ids = {dep for r in roots for dep in r.get('dependencies', [])}
    variants = {
        'logical_surface_bodies.h': ([m for m in members if m['id'] in direct_ids], True),
        'logical_closure_bodies.h': (chosen, True),
        'logical_declarations.h': ([m for m in chosen if m['kind'] != 'Field'], False),
    }
    for name, (items, bodies) in variants.items():
        text = ('// Automatically sliced from the pinned LLVM source; bounded pilot.\n'
                '// LLVM source fragments: Apache-2.0 WITH LLVM-exception.\n'
                '#pragma once\nnamespace llvm {\nstruct [[nodiscard]] LogicalResult {\n')
        for item in items:
            text += item['access'] + ':\n' + declaration(item, bodies) + '\n'
        text += '};\n'
        text += '\n'.join(declaration(r, bodies) for r in roots) + '\n}\n'
        (out / name).write_text(text)
    return {'real_explicit_member_count': len(members),
            'closure_member_count': len(chosen),
            'selected_members': [{'name': m['name'], 'id': m['id'],
                                  'kind': m['kind'], 'dependencies': m.get('dependencies', [])}
                                 for m in chosen],
            'unresolved_edges': [], 'root_ids': [r['id'] for r in roots]}


def array_models(probe: dict, out: Path) -> dict:
    """选择真实 ArrayRef 的固定成员集，控制字段、函数体、宏和模板约束。

    标准库依赖头由实验配置人工列出，类模板壳固定为 template<class T>；
    这不是任意模板、别名、宏与类型依赖的自动闭包算法。
    """
    members = probe['library']['ArrayRef']['members']
    selected = []
    for m in members:
        short = m.get('name', '').split('::')[-1]
        signature = m.get('signature', '')
        constructor = m.get('constructor', False)
        if short in {'Data', 'Length', 'size_type', 'size', 'data', 'operator[]'}:
            selected.append(m)
        elif constructor and (not m.get('parameter_types') or signature in {'void ()', 'void () noexcept',
                                             'void (const T &)', 'void (const T *, size_t)',
                                             'void (const C &)'}):
            selected.append(m)
    if not any(m.get('template') for m in selected):
        raise ValueError('Generic ArrayRef constructor was not extracted')
    replacements = {m['replacement'] for m in probe['macros']
                    if m['name'] == 'LLVM_LIFETIME_BOUND' and not m['function_like']}
    if len(replacements) != 1:
        raise ValueError('Lifetime macro snapshot is absent or ambiguous')
    macro = next(iter(replacements))
    variants = {
        'array_raw.h': (True, True, False, False),
        'array_closed.h': (True, True, True, False),
        'array_no_fields.h': (True, False, True, False),
        'array_declarations.h': (False, True, True, False),
        'array_unconstrained_declarations.h': (False, True, True, True),
    }
    for name, (bodies, fields, macros, erase) in variants.items():
        text = ('// LLVM source fragments: Apache-2.0 WITH LLVM-exception.\n#pragma once\n'
                '#include <cstddef>\n#include <type_traits>\n#include <utility>\n#include <cassert>\n')
        if macros:
            text += '#define LLVM_LIFETIME_BOUND ' + macro + '\n'
        text += 'namespace llvm {\ntemplate<typename T> class ArrayRef {\n'
        for m in selected:
            if m['kind'] == 'Field' and not fields:
                continue
            code = declaration(m, bodies)
            if erase and m.get('template'):
                # 人为消融对照：明确丢弃约束，只保留第一模板参数和真实函数签名。
                code = 'template<typename C>\nconstexpr ArrayRef(const C &V);'
            text += m['access'] + ':\n' + code + '\n'
        text += '};\n}\n'
        if macros:
            text += '#undef LLVM_LIFETIME_BOUND\n'
        (out / name).write_text(text)
    return {'real_explicit_member_count': len(members),
            'selected_member_count': len(selected),
            'selected_members': [{'name': m['name'], 'kind': m['kind'],
                                  'signature': m.get('signature'), 'id': m['id']}
                                 for m in selected],
            'lifetime_macro_replacement': macro,
            'manual_dependency_seeds': ['cstddef', 'type_traits', 'utility', 'cassert'],
            'limits': ['Only fixed primary-template shell and selected members',
                       'Unconstrained variant deliberately changes template availability']}
