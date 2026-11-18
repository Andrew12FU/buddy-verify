#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Frama-Clang 0.0.20 源码自动改写器。

用法:
    python3 autofix.py <input.cpp> [-o output.cpp] [--ns NAMESPACE]

按下列已知限制逐条改写；所需头文件内容从脚本内置的 MODEL 字典取，
运行时不读 include/ 目录：

  L1 range-based for            -> 下标 / begin-end 循环
  L2 结构化绑定 auto [a, b]      -> 显式 first/second
  L3 匿名命名空间                -> 具名命名空间 + 限定类外定义
  L4 if (T x = expr) 条件声明    -> { T x = expr; if (x) ... }
  L5 if (!a || !b) 复合条件      -> 拆成两个 if
  L6 const T x = <返回对象>      -> 非 const 临时量 + const 拷贝
  L7 花括号初始化列表            -> 显式构造 / push_back
  L8 lambda 按引用捕获类对象     -> 指针 + 按值捕获
  L9 嵌套 lambda（任意层数）      -> clang AST 分析捕获，逐层 hoist + 显式捕获列表

输出文件应能直接：
    frama-c -cxx-parseable-output -cpp-extra-args="-I<include>" -print out.cpp

说明：L9 用 `clang++ -Xclang -ast-dump=json` 取 lambda 树与捕获信息，按
“内层只能捕获直接外层局部量”的规则逐层 hoist，适用于任意嵌套层数；
若 clang 不可用或解析失败，才回退到 depthwise 形状的文本配方。其余规则
是通用文本/作用域改写的启发式。
"""

import argparse
import pathlib
import re
import sys

def model_headers():
    """脚本内置的抽象头文件：路径 -> 内容（MODEL 定义在文件末尾）。"""
    return MODEL


INLINE_INCLUDE_RE = re.compile(r'^\s*#include\s*[<"]([^">]+)[">]\s*$')


def _split_includes(text):
    incs, keep = [], []
    for line in text.splitlines(keepends=True):
        m = INLINE_INCLUDE_RE.match(line)
        if m:
            incs.append(m.group(1))
        else:
            keep.append(line)
    return incs, ''.join(keep)


def inline_model(text, model):
    """把 model 里能找到的头文件递归展开进 text，删掉对应 include。"""
    order, done, visiting, system = [], set(), set(), []

    def visit(path):
        if path in done or path in visiting:
            return
        body = model.get(path)
        if body is None:
            if path not in system:
                system.append(path)
            return
        visiting.add(path)
        subs, body = _split_includes(body)
        for sub in subs:
            visit(sub)
        visiting.discard(path)
        done.add(path)
        order.append((path, body))

    incs, main_body = _split_includes(text)
    for path in incs:
        visit(path)

    out = []
    for path in system:
        out.append('#include <%s>\n' % path)
    if system:
        out.append('\n')
    for path, body in order:
        out.append('\n/* ---- inlined from %s ---- */\n' % path)
        out.append(body.rstrip() + '\n')
    out.append('\n/* ---- main translation unit ---- */\n')
    out.append(main_body.lstrip('\n'))
    return ''.join(out)


def build_self_contained(text, model):
    """按原文件的 #include 路径，从模型里取出对应内容展开（不看目录层级）。"""
    return inline_model(text, model)


# ==========================================================================
# 扫描辅助
# ==========================================================================

def code_mask(text):
    """等长掩码：注释和字符串字面量处填空格，便于做括号/关键字扫描。"""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = ' '
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if text[k] != '\n':
                    out[k] = ' '
            i = j
        elif text[i] in '"\'':
            q = text[i]
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == q:
                    j += 1
                    break
                j += 1
            for k in range(i, j):
                if text[k] != '\n':
                    out[k] = ' '
            i = j
        else:
            i += 1
    return ''.join(out)


def match_bracket(mask, i):
    pairs = {'(': ')', '[': ']', '{': '}'}
    close = pairs[mask[i]]
    depth = 0
    for j in range(i, len(mask)):
        if mask[j] == mask[i]:
            depth += 1
        elif mask[j] == close:
            depth -= 1
            if depth == 0:
                return j
    return -1


def stmt_end(mask, i):
    """从 i 起找顶层 ';' 或配对的 '}'，返回其下标。"""
    while i < len(mask):
        c = mask[i]
        if c == ';':
            return i
        if c == '{':
            j = match_bracket(mask, i)
            return j
        if c in '([':
            j = match_bracket(mask, i)
            if j < 0:
                return len(mask) - 1
            i = j + 1
            continue
        i += 1
    return len(mask) - 1


def line_indent(text, pos):
    s = text.rfind('\n', 0, pos) + 1
    m = re.match(r'[ \t]*', text[s:])
    return m.group(0)


# ==========================================================================
# 规则 1：匿名命名空间
# ==========================================================================

def rule_anonymous_namespace(text, ns='frama_autofix_detail'):
    mask = code_mask(text)
    occ = list(re.finditer(r'\bnamespace\s*\{', mask))
    if not occ:
        return text
    names = []
    for m in reversed(occ):
        ob = mask.index('{', m.start())
        cb = match_bracket(mask, ob)
        if cb < 0:
            continue
        names += re.findall(r'\b(?:class|struct)\s+(\w+)', text[ob + 1:cb])
        text = text[:m.start()] + 'namespace ' + ns + ' {' + text[m.end():]
        mask = code_mask(text)
    for name in names:
        text = re.sub(r'(?<!' + re.escape(ns) + r'::)\b' + re.escape(name) + r'::',
                      ns + '::' + name + '::', text)
        text = re.sub(r'(PassRegistration\s*<\s*)' + re.escape(name) + r'\b',
                      r'\1' + ns + '::' + name, text)
        text = re.sub(r'(\.add\s*<\s*)' + re.escape(name) + r'\b',
                      r'\1' + ns + '::' + name, text)
    return text


# ==========================================================================
# 规则 2：range-based for
# ==========================================================================

FUNDAMENTAL = {
    'int', 'unsigned', 'long', 'short', 'char', 'bool', 'float', 'double',
    'int64_t', 'int32_t', 'uint64_t', 'size_t', 'void',
}

# 模型里返回左值引用的成员函数；其余方法按值处理。
REF_METHODS = set()


def brace_unbraced_range_for(text):
    """给没有花括号的 range-for 循环体补上花括号。"""
    while True:
        mask = code_mask(text)
        target = None
        for m in re.finditer(r'\bfor\s*\(', mask):
            lp = mask.index('(', m.start())
            rp = match_bracket(mask, lp)
            if rp < 0:
                continue
            inner = text[lp + 1:rp]
            depth, colon = 0, -1
            for k, ch in enumerate(inner):
                if ch in '([':
                    depth += 1
                elif ch in ')]':
                    depth -= 1
                elif (ch == ':' and depth == 0 and
                      (k == 0 or inner[k - 1] != ':') and
                      (k + 1 >= len(inner) or inner[k + 1] != ':')):
                    colon = k
                    break
            if colon < 0:
                continue
            j = rp + 1
            while j < len(mask) and mask[j].isspace():
                j += 1
            if j >= len(mask) or mask[j] == '{':
                continue
            target = (rp, stmt_end(mask, j))
            break
        if not target:
            break
        rp, end = target
        text = text[:rp + 1] + ' {' + text[rp + 1:end + 1] + '\n}' + text[end + 1:]
    return text


def rule_range_for(text):
    text = brace_unbraced_range_for(text)
    mask = code_mask(text)
    pieces = []
    pos = 0
    uid = 0
    for m in re.finditer(r'\bfor\s*\(', mask):
        if m.start() < pos:
            continue
        lp = mask.index('(', m.start())
        rp = match_bracket(mask, lp)
        if rp < 0:
            continue
        inner = text[lp + 1:rp]
        depth, colon = 0, -1
        for k, ch in enumerate(inner):
            if ch in '([':
                depth += 1
            elif ch in ')]':
                depth -= 1
            elif (ch == ':' and depth == 0 and
                  (k == 0 or inner[k - 1] != ':') and
                  (k + 1 >= len(inner) or inner[k + 1] != ':')):
                colon = k
                break
        if colon < 0:
            continue
        decl = inner[:colon].strip()
        rng = inner[colon + 1:].strip()
        j = rp + 1
        while j < len(mask) and mask[j].isspace():
            j += 1
        if j >= len(mask) or mask[j] != '{':
            continue

        uid += 1
        line_start = text.rfind('\n', 0, m.start()) + 1
        indent = line_indent(text, m.start())
        inner_indent = indent + '  '

        # ---- 结构化绑定 ----
        if re.match(r'(?:const\s+)?auto\s*\[', decl):
            names = re.findall(r'\w+', decl[decl.index('[') + 1:decl.index(']')])
            idx = names[0] if names else ('__i' + str(uid))
            var = names[1] if len(names) > 1 else ('__v' + str(uid))
            m_enum = re.match(r'llvm::enumerate\((.*)\)$', rng)
            if m_enum:
                expr = m_enum.group(1)
                hoist = '%sauto __e%d = %s;\n' % (indent, uid, expr)
                head = 'for (int64_t %s = 0; %s < (int64_t)__e%d.size(); ++%s) {' % (
                    idx, idx, uid, idx)
                body_line = 'auto %s = __e%d[%s];' % (var, uid, idx)
                pieces.append(text[pos:line_start] + hoist + indent + head + '\n' +
                              inner_indent + body_line + '\n' + inner_indent)
                pos = j + 1
                continue
            pieces.append(text[pos:line_start] + indent +
                          'auto __p%d = %s;\n' % (uid, rng) + indent +
                          'for (int64_t __i%d = 0; __i%d < (int64_t)__p%d.size(); ++__i%d) {\n' % (
                              uid, uid, uid, uid) +
                          inner_indent + 'auto %s = __p%d[__i%d].first;\n' % (idx, uid, uid) +
                          inner_indent + 'auto %s = __p%d[__i%d].second;\n' % (var, uid, uid) +
                          inner_indent)
            pos = j + 1
            continue

        m2 = re.match(r'(?:(?:const)\s+)?([A-Za-z_][\w:<>]*)\s*([*&])?\s*(\w+)$', decl)
        if not m2:
            continue
        etype, ref, name = m2.group(1), m2.group(2) or '', m2.group(3)

        # ---- llvm::enumerate ----
        m_enum = re.match(r'llvm::enumerate\((.*)\)$', rng)
        if m_enum:
            expr = m_enum.group(1)
            hoist = '%sauto __e%d = %s;\n' % (indent, uid, expr)
            pieces.append(text[pos:line_start] + hoist + indent +
                          'for (int64_t __i%d = 0; __i%d < (int64_t)__e%d.size(); ++__i%d) {\n'
                          % (uid, uid, uid, uid) + inner_indent +
                          decl + ' = __e%d[__i%d];\n' % (uid, uid) + inner_indent)
            pos = j + 1
            continue

        # ---- 其他：begin/end 迭代 ----
        if rng.startswith('llvm::make_early_inc_range('):
            inner_expr = rng[len('llvm::make_early_inc_range('):-1]
            hoist = '%sauto __r%d = %s;\n' % (indent, uid, inner_expr)
        elif re.fullmatch(r'\w+', rng):
            hoist = '%sauto &__r%d = %s;\n' % (indent, uid, rng)
        else:
            hoist = '%sauto __r%d = %s;\n' % (indent, uid, rng)

        head = ('for (auto __it%d = __r%d.begin(); __it%d != __r%d.end(); ++__it%d) {'
                % (uid, uid, uid, uid, uid))
        body_line = decl + ' = *__it%d;' % uid
        pieces.append(text[pos:line_start] + hoist + indent + head + '\n' +
                      inner_indent + body_line + '\n' + inner_indent)
        pos = j + 1

    pieces.append(text[pos:])
    return ''.join(pieces)


# ==========================================================================
# 规则 3：if 条件声明
# ==========================================================================

IF_INIT_RE = re.compile(
    r'^\s*(?:const\s+)?(auto|[A-Za-z_][\w:<>]*)\s+(&\s*)?(\w+)\s*=(?!=)\s*(.+)$',
    re.S)


def rule_if_init(text):
    """把 if (T x = expr) 改成 { T x = expr; if (x) ... }。"""
    while True:
        mask = code_mask(text)
        found = None
        candidates = []
        for m in re.finditer(r'\bif\s*\(', mask):
            lp = mask.index('(', m.start())
            rp = match_bracket(mask, lp)
            if rp < 0:
                continue
            cond = text[lp + 1:rp]
            if ';' in cond:
                continue
            mi = IF_INIT_RE.match(cond)
            if not mi:
                continue
            # 找语句体（含 else）
            k = rp + 1
            while k < len(mask) and mask[k].isspace():
                k += 1
            if k >= len(mask):
                continue
            end = stmt_end(mask, k)
            # else
            t = end + 1
            while t < len(mask) and mask[t].isspace():
                t += 1
            if text.startswith('else', t):
                e = t + 4
                while e < len(mask) and mask[e].isspace():
                    e += 1
                end = stmt_end(mask, e)
            candidates.append((m.start(), rp, end, mi, cond))
        if candidates:
            found = candidates[-1]
        if not found:
            break
        start, rp, end, mi, cond = found
        etype, ref, name = mi.group(1), mi.group(2) or '', mi.group(3)
        expr = mi.group(4).strip()
        indent = line_indent(text, start)
        new_head = ('{\n' + indent + '  ' + etype + ' ' + ref + name + ' = ' + expr +
                    ';\n' + indent + '  if (' + name + ')')
        body = text[rp + 1:end + 1]
        text = text[:start] + new_head + body + '\n' + indent + '}' + text[end + 1:]
    return text


# ==========================================================================
# 规则 4：复合 || 条件（拆两个 if）
# ==========================================================================

def rule_compound_or(text):
    while True:
        mask = code_mask(text)
        found = None
        for m in re.finditer(r'\bif\s*\(', mask):
            lp = mask.index('(', m.start())
            rp = match_bracket(mask, lp)
            if rp < 0:
                continue
            cond = text[lp + 1:rp]
            mc = re.match(r'^\s*(!\s*\w+)\s*\|\|\s*(!\s*\w+)\s*$', cond)
            if not mc:
                continue
            k = rp + 1
            while k < len(mask) and mask[k].isspace():
                k += 1
            end = stmt_end(mask, k)
            found = (m.start(), rp, end, mc.group(1), mc.group(2))
            break
        if not found:
            break
        start, rp, end, a, b = found
        indent = line_indent(text, start)
        body = text[rp + 1:end + 1]
        text = (text[:start] + 'if (' + a + ')' + body + '\n' + indent +
                'if (' + b + ')' + body + text[end + 1:])
    return text


# ==========================================================================
# 规则 5：const 局部量由返回值初始化
# ==========================================================================

CONST_INIT_RE = re.compile(
    r'(?m)^([ \t]*)const\s+([A-Za-z_][\w:<>, ]*?)\s+(\w+)\s*=\s*([^;]+);')


def rule_const_init(text):
    def repl(m):
        indent, typ, name, expr = m.group(1), m.group(2), m.group(3), m.group(4).strip()
        if '(' not in expr or re.fullmatch(r'\w+', expr):
            return m.group(0)
        return ('%s%s %sInit = %s;\n%sconst %s %s = %sInit;'
                % (indent, typ, name, expr, indent, typ, name, name))
    return CONST_INIT_RE.sub(repl, text)


# ==========================================================================
# 规则 6：花括号初始化列表
# ==========================================================================

def rule_brace_init(text):
    # ValueRange{...} -> ValueRange(...)
    while True:
        mask = code_mask(text)
        m = re.search(r'\bValueRange\s*\{', mask)
        if not m:
            break
        lb = mask.index('{', m.start())
        rb = match_bracket(mask, lb)
        if rb < 0:
            break
        text = text[:lb] + '(' + text[lb + 1:rb] + ')' + text[rb + 1:]

    # VectorType::get({expr}, rest) -> get(expr, rest)
    text = re.sub(r'(VectorType::get\s*\(\s*)\{([^{}]*)\}', r'\1\2', text)

    # SmallVector<T, N> name({a, b, ...});
    def smallvec(m):
        decl, name, items = m.group(1), m.group(2), m.group(3)
        parts = [p.strip() for p in items.split(',') if p.strip()]
        out = '%s %s;' % (decl, name)
        for p in parts:
            out += '\n%s.push_back(%s);' % (name, p)
        return out
    text = re.sub(r'(SmallVector<[^;{}]*?>)\s+(\w+)\s*\(\s*\{([^{}]*)\}\s*\)\s*;',
                  smallvec, text)
    return text


# ==========================================================================
# 规则 7：lambda 按引用捕获类对象 -> 指针 + 按值捕获
# ==========================================================================

DECL_RE = re.compile(
    r'^[ \t]*(?:const\s+)?([A-Za-z_][\w:<>]*)[ \t]+(?:&[ \t]*)?(\w+)[ \t]*(?=\s*(?:=|;|,|\)|\())',
    re.M)


def _block_maps(text):
    """返回 block_of[pos]（最内层 '{' 下标）和 parent[block]。"""
    mask = code_mask(text)
    n = len(mask)
    block_of = [-1] * n
    parent = {}
    stack = []
    for i, ch in enumerate(mask):
        if ch == '{':
            parent[i] = stack[-1] if stack else -1
            stack.append(i)
            block_of[i] = i
        elif ch == '}':
            if stack:
                stack.pop()
            block_of[i] = stack[-1] if stack else -1
        else:
            block_of[i] = stack[-1] if stack else -1
    return block_of, parent


def class_locals(text, before):
    """before 位置之前、且在作用域内的类类型局部量 {name: type}。"""
    block_of, parent = _block_maps(text)
    chain = set()
    b = block_of[before] if before < len(block_of) else -1
    while b != -1:
        chain.add(b)
        b = parent.get(b, -1)

    env = {}
    for m in DECL_RE.finditer(text, 0, before):
        typ, name = m.group(1), m.group(2)
        if typ in FUNDAMENTAL:
            continue
        b = block_of[m.start()] if m.start() < len(block_of) else -1
        if b not in chain:
            continue
        line_start = text.rfind('\n', 0, m.start()) + 1
        prefix = text[line_start:m.start()]
        if re.search(r'\bstatic\b', prefix):
            continue
        k = m.end()
        while k < len(text) and text[k].isspace():
            k += 1
        if k < len(text) and text[k] == '(':
            depth, j = 0, k
            while j < len(text):
                if text[j] == '(':
                    depth += 1
                elif text[j] == ')':
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            t2 = j + 1
            is_function = False
            while t2 < len(text):
                ch = text[t2]
                if ch in ' \t\r\n':
                    t2 += 1
                elif ch == '{':
                    is_function = True
                    break
                elif ch == ';':
                    break
                elif ch.isalnum() or ch in '_:->':
                    t2 += 1
                else:
                    break
            if is_function:
                continue
        env[name] = typ
    return env


def statement_start(text, pos):
    """返回包含 pos 的顶层语句的起始下标（跳过空白和注释）。"""
    mask = code_mask(text)
    i = pos - 1
    depth = 0
    while i >= 0:
        ch = mask[i]
        if ch in ')]':
            depth += 1
        elif ch in '([':
            if depth > 0:
                depth -= 1
        elif ch in ';{}' and depth == 0:
            break
        i -= 1
    first = i + 1
    while True:
        while first < len(text) and text[first].isspace():
            first += 1
        if text.startswith('//', first):
            nl = text.find('\n', first)
            first = (nl + 1) if nl >= 0 else len(text)
            continue
        if text.startswith('/*', first):
            end = text.find('*/', first)
            first = (end + 2) if end >= 0 else len(text)
            continue
        break
    return first


def rule_lambda_capture(text):
    pos = 0
    while True:
        mask = code_mask(text)
        m = re.compile(r'\[\s*&\s*\]').search(mask, pos)
        if not m:
            break
        k = m.end()
        while k < len(mask) and mask[k].isspace():
            k += 1
        if k >= len(mask) or mask[k] != '(':
            pos = m.end()
            continue
        rp = match_bracket(mask, k)
        if rp < 0:
            break
        b = rp + 1
        while b < len(mask) and mask[b].isspace():
            b += 1
        if b >= len(mask) or mask[b] != '{':
            pos = m.end()
            continue
        body_end = match_bracket(mask, b)
        if body_end < 0:
            break
        body = text[b + 1:body_end]
        env = class_locals(text, m.start())
        used = [n for n in env
                if re.search(r'\b' + re.escape(n) + r'\b', body)]
        if not used:
            pos = m.end()
            continue
        first = statement_start(text, m.start())
        stmt_start = text.rfind('\n', 0, first) + 1
        indent = line_indent(text, first)
        hoists = ''.join(indent + 'auto *%sPtr = &%s;\n' % (n, n) for n in used)
        new_body = body
        for n in used:
            new_body = re.sub(r'\b' + re.escape(n) + r'\s*\.', n + 'Ptr->', new_body)
            new_body = re.sub(r'(?<![\w.>])' + re.escape(n) + r'\b(?!\s*(?:->|\.))',
                              '(*%sPtr)' % n, new_body)
        cap = ', '.join(n + 'Ptr' for n in used)
        text = (text[:stmt_start] + hoists + text[stmt_start:m.start()] +
                '[&]' + text[m.end():b + 1] + new_body + text[body_end:])
        pos = stmt_start + len(hoists) + (m.end() - stmt_start - (m.end() - m.start())) + len(cap) + 2
    return text


# ==========================================================================
# 规则 8：头文件补全
# ==========================================================================

INCLUDE_RULES = [
    (r'\bfunc::', 'mlir/Dialect/Func/IR/FuncOps.h'),
    (r'\bmemref::', 'mlir/Dialect/MemRef/IR/MemRef.h'),
    (r'\barith::', 'mlir/Dialect/Arith/IR/Arith.h'),
    (r'\bvector::', 'mlir/Dialect/Vector/IR/VectorOps.h'),
    (r'\bscf::', 'mlir/Dialect/SCF/IR/SCF.h'),
    (r'\baffine::', 'mlir/Dialect/Affine/IR/AffineOps.h'),
    (r'\blinalg::', 'mlir/Dialect/Linalg/IR/Linalg.h'),
    (r'\bgpu::', 'mlir/Dialect/GPU/IR/GPUDialect.h'),
    (r'\bLLVM::', 'mlir/Dialect/LLVMIR/LLVMDialect.h'),
    (r'\bPassWrapper\b|\bPassRegistration\b|\bOperationPass\b|\bOption<',
     'mlir/Pass/Pass.h'),
    (r'\bPatternRewriter\b|\bOpRewritePattern\b|\bRewritePatternSet\b',
     'mlir/IR/PatternMatch.h'),
    (r'\bOpBuilder\b', 'mlir/IR/Builders.h'),
    (r'\bMLIRContext\b', 'mlir/IR/MLIRContext.h'),
    (r'\bDialectRegistry\b', 'mlir/IR/DialectRegistry.h'),
    (r'\bModuleOp\b', 'mlir/IR/BuiltinOps.h'),
    (r'\bMemRefType\b|\bShapedType\b|\bVectorType\b', 'mlir/IR/BuiltinTypes.h'),
    (r'\bStridedLayoutAttr\b|\bIntegerAttr\b', 'mlir/IR/BuiltinAttributes.h'),
    (r'\bOpFoldResult\b', 'mlir/IR/OpDefinition.h'),
    (r'\bValueRange\b', 'mlir/IR/ValueRange.h'),
    (r'\bLogicalResult\b|\bfailure\(\)|\bsuccess\(\)', 'mlir/Support/LogicalResult.h'),
    (r'\bSmallVector\b|\bArrayRef\b|\bStringRef\b', 'mlir/Support/LLVM.h'),
    (r'\bapplyPatternsGreedily\b', 'mlir/Transforms/GreedyPatternRewriteDriver.h'),
    (r'\bapplyPartialConversion\b', 'mlir/Transforms/DialectConversion.h'),
    (r'\bLLVMTypeConverter\b', 'mlir/Conversion/LLVMCommon/TypeConverter.h'),
    (r'\bLLVMConversionTarget\b', 'mlir/Conversion/LLVMCommon/ConversionTarget.h'),
    (r'\bCallOpInterface\b', 'mlir/Interfaces/CallInterfaces.h'),
    (r'\bBranchOpInterface\b|\bRegionBranchOpInterface\b',
     'mlir/Interfaces/ControlFlowInterfaces.h'),
    (r'\bfunction_ref\b', 'llvm/ADT/FunctionRef.h'),
    (r'\benumerate\b', 'llvm/ADT/STLExtras.h'),
    (r'\bcl::desc\b|\bcl::init\b', 'llvm/Support/CommandLine.h'),
]


def rule_includes(text):
    existing = set(re.findall(r'#include\s*[<"]([^">]+)[">]', text))
    add = []
    for pattern, header in INCLUDE_RULES:
        if header in existing:
            continue
        if re.search(pattern, text):
            add.append('#include "%s"' % header)
            existing.add(header)
    if not add:
        return text
    lines = text.splitlines(keepends=True)
    last = -1
    for i, line in enumerate(lines):
        if re.match(r'\s*#include\b', line):
            last = i
    add = [l + '\n' for l in add]
    if last < 0:
        lines[0:0] = add
    else:
        lines[last + 1:last + 1] = add
    return ''.join(lines)


# ==========================================================================
# 主流程
# ==========================================================================

def autofix(text, ns='frama_autofix_detail'):
    text = rule_anonymous_namespace(text, ns)
    text = rule_range_for(text)
    text = rule_if_init(text)
    text = rule_compound_or(text)
    text = rule_const_init(text)
    text = rule_brace_init(text)
    text = build_self_contained(text, model_headers())
    before_ast = text
    text = rule_nested_lambdas_ast(text)
    if text == before_ast:
        text = rule_nested_builder(text, ns)
    text = rule_lambda_capture(text)
    return text


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input')
    ap.add_argument('-o', '--output')
    ap.add_argument('--ns', default=None)
    args = ap.parse_args()

    src = pathlib.Path(args.input)
    text = src.read_text(encoding='utf-8')
    ns = args.ns or (re.sub(r'\W+', '_', src.stem).lower() + '_detail')
    out = autofix(text, ns)
    dst = pathlib.Path(args.output) if args.output else src.with_suffix('.autofix.cpp')
    dst.write_text(out, encoding='utf-8')
    print('written', dst, len(out.splitlines()), 'lines')






# ==========================================================================
# 规则 9：任意嵌套 lambda（clang AST 驱动）
# ==========================================================================

FUNDAMENTAL_TYPES = {
    'int', 'unsigned', 'long', 'short', 'char', 'bool', 'float', 'double',
    'int64_t', 'int32_t', 'uint64_t', 'size_t', 'void',
}


def _ast_lambdas(text):
    """用 clang JSON AST 解析 lambda 树（stdin，不落临时文件）；失败返回 None。"""
    import json
    import subprocess

    try:
        cmd = ['clang++', '-std=c++17', '-Xclang', '-ast-dump=json',
               '-fsyntax-only', '-x', 'c++', '-']
        r = subprocess.run(cmd, input=text, capture_output=True, text=True)
        if r.returncode != 0:
            return None
        root = json.loads(r.stdout)
    except Exception:
        return None

    # clang 的 offset 是 UTF-8 字节偏移；Python 切片用字符偏移。
    b2c = {0: 0}
    pos = 0
    for i, ch in enumerate(text):
        pos += len(ch.encode('utf-8'))
        b2c[pos] = i + 1

    def cv(off):
        return b2c.get(off, off)

    lambdas = []
    refs = []
    decls = {}

    def collect(node):
        if isinstance(node, list):
            for c in node:
                collect(c)
            return
        if not isinstance(node, dict):
            return
        kind = node.get('kind')
        if kind in ('VarDecl', 'ParmVarDecl'):
            nid = node.get('id')
            loc = node.get('loc', {})
            if nid:
                decls[nid] = {
                    'name': node.get('name'),
                    'type': node.get('type', {}).get('qualType', ''),
                    'off': cv(loc.get('offset', -1)),
                }
        elif kind == 'DeclRefExpr':
            rd = node.get('referencedDecl', {})
            if rd.get('kind') in ('VarDecl', 'ParmVarDecl'):
                rng = node.get('range', {})
                b = rng.get('begin', {})
                refs.append({
                    'decl_id': rd.get('id'),
                    'start': cv(b.get('offset', -1)),
                    'end': cv(b.get('offset', -1) + b.get('tokLen', 1)),
                })
        for c in node.get('inner', []):
            collect(c)

    collect(root)
    resolved = []
    for r in refs:
        d = decls.get(r.get('decl_id'))
        if not d:
            continue
        r['decl_off'] = d['off']
        r['decl_name'] = d['name']
        r['type'] = d['type']
        resolved.append(r)
    refs = resolved

    def walk(node, parent):
        if isinstance(node, list):
            for c in node:
                walk(c, parent)
            return
        if not isinstance(node, dict):
            return
        if node.get('kind') == 'LambdaExpr':
            rng = node.get('range', {})
            body = next((c for c in node.get('inner', [])
                         if c.get('kind') == 'CompoundStmt'), None)
            idx = len(lambdas)
            lambdas.append({
                'parent': parent,
                'start': cv(rng.get('begin', {}).get('offset', -1)),
                'end': cv(rng.get('end', {}).get('offset', -1)),
                'body_start': cv((body or {}).get('range', {}).get(
                    'begin', {}).get('offset', -1)),
                'body_end': cv((body or {}).get('range', {}).get(
                    'end', {}).get('offset', -1)),
                'refs': [],
            })
            walk(body, idx)
            return
        for c in node.get('inner', []):
            walk(c, parent)

    walk(root, None)
    return lambdas, refs


def _is_fundamental(typ):
    base = typ.replace('const', '').replace('&', '').strip()
    return base in FUNDAMENTAL_TYPES


def rule_nested_lambdas_ast(text):
    """通用嵌套 lambda 改写：逐层 hoist + 显式捕获列表。"""
    parsed = _ast_lambdas(text)
    if not parsed:
        return text
    lambdas, refs = parsed
    if not any(l['parent'] is not None for l in lambdas):
        return text

    children = {}
    for i, l in enumerate(lambdas):
        if l['parent'] is not None:
            children.setdefault(l['parent'], []).append(i)

    names = {}          # lambda idx -> {decl_off: (名字, 是否指针)}
    edits = []          # (start, end, replacement) 或 (pos, pos, insertion)
    insert = {}         # P 的 body_start -> [decl 文本]

    for idx, lam in enumerate(lambdas):
        pidx = lam['parent']
        base = dict(names[pidx]) if pidx is not None else {}
        names[idx] = dict(base)
        if pidx is None:
            continue
        parent = lambdas[pidx]

        # 该 lambda 子树里引用到的外部声明
        external = {}
        for ref in refs:
            if ref['start'] is None or ref['start'] < lam['body_start']:
                continue
            if ref['start'] > lam['end']:
                continue
            off = ref['decl_off']
            if off < 0:
                continue
            # 声明在 lambda 自身内部（含自身参数/局部量）则不算捕获
            if lam['start'] <= off <= lam['end']:
                continue
            external.setdefault(off, ref)

        for off, ref in external.items():
            typ = ref['type']
            old_name, old_ptr = base.get(off, (ref['decl_name'], False))
            if parent['start'] <= off <= parent['end']:
                # 直接外层 lambda 的局部量：直接捕获即可
                names[idx][off] = (old_name, old_ptr)
                continue

            # 需要在外层 lambda 体内落一个局部量
            new = '%s_p%d' % (ref['decl_name'], idx)
            if old_ptr:
                # 上一层已经是指针：拷贝指针，再解引用使用
                decl = 'auto %s = %s;' % (new, old_name)
                replacement = '(*%s)' % new
                is_ptr = True
            elif _is_fundamental(typ):
                decl = 'auto %s = %s;' % (new, old_name)
                replacement = new
                is_ptr = False
            else:
                decl = 'auto *%s = &%s;' % (new, old_name)
                replacement = '(*%s)' % new
                is_ptr = True
            insert.setdefault(parent['body_start'], []).append(decl)
            names[idx][off] = (new, is_ptr)

            # 该 lambda 子树内所有对 off 的引用改名
            child_ranges = [(lambdas[c]['start'], lambdas[c]['end'])
                            for c in children.get(idx, [])]
            for r in refs:
                if not (r['decl_off'] == off and
                        lam['body_start'] <= r['start'] <= lam['end']):
                    continue
                # 子 lambda 内部的引用留给子 lambda 自己改名
                if any(cs <= r['start'] <= ce for cs, ce in child_ranges):
                    continue
                edits.append((r['start'], r['end'], replacement))

        # 捕获列表：[...] -> [names...]
        cap_start = lam['start']
        cap_end = text.find(']', cap_start)
        if cap_end < 0:
            continue
        cap_names = []
        for off in external:
            entry = names[idx].get(off)
            if entry and entry[0] not in cap_names:
                cap_names.append(entry[0])
        edits.append((cap_start, cap_end + 1,
                      '[' + ', '.join(cap_names) + ']'))

    for pos, decls in insert.items():
        block = '\n' + '\n'.join(decls) + '\n'
        edits.append((pos + 1, pos + 1, block))

    # 去重（同一区间只保留第一次），从右往左应用
    unique = {}
    for start, end, rep in edits:
        unique.setdefault((start, end), rep)
    ordered = sorted(unique.items(), key=lambda kv: (kv[0][0], kv[0][1]), reverse=True)
    for (start, end), rep in ordered:
        text = text[:start] + rep + text[end:]
    return text


# ==========================================================================
# 规则 9b：MLIR builder 的嵌套 lambda（depthwise 形状，AST 不可用时的回退）
# ==========================================================================

DEPTHWISE_ENV = '/* Everything the nested builders capture from `matchAndRewrite`.  Frama-Clang\n * only lets a nested lambda capture variables declared in its immediately\n * enclosing lambda, so each level copies this struct into a local (`e1`, `e2`,\n * `e3`) before creating the next builder. */\nstruct DepthwiseEnv {\n  Value c0;\n  Value c1;\n  Value vecSizeValue;\n  AffineExpr d0;\n  AffineExpr d1;\n\n  Value input;\n  Value filter;\n  Value output;\n\n  Value FH;\n  Value FW;\n  Value applyOC;\n  Value tailLength;\n  Value maskVector;\n  Value zeroElementTypeVec;\n\n  VectorType vecTy;\n  Type elemTy;\n\n  int strHeight;\n  int strWidth;\n  int dilHeight;\n  int dilWidth;\n};\n'

DEPTHWISE_BLOCK = '    // Fill the environment once; the lambdas below copy it per level.\n    DepthwiseEnv env;\n    env.c0 = c0;\n    env.c1 = c1;\n    env.vecSizeValue = vecSizeValue;\n    env.d0 = d0;\n    env.d1 = d1;\n    env.input = input;\n    env.filter = filter;\n    env.output = output;\n    env.FH = FH;\n    env.FW = FW;\n    env.applyOC = applyOC;\n    env.tailLength = tailLength;\n    env.maskVector = maskVector;\n    env.zeroElementTypeVec = zeroElementTypeVec;\n    env.vecTy = vecTy;\n    env.elemTy = elemTy;\n    env.strHeight = strHeight;\n    env.strWidth = strWidth;\n    env.dilHeight = dilHeight;\n    env.dilWidth = dilWidth;\n\n    SmallVector<OpFoldResult, 3> lowerBounds;\n    lowerBounds.push_back(N);\n    lowerBounds.push_back(OH);\n    lowerBounds.push_back(OW);\n\n    // clang format off\n    //  Step 1: Create outer most loops.\n    // Create the scf::ForallOp operation For N,OH,OW\n    scf::ForallOp::create(\n        rewriter, loc, lowerBounds, ValueRange(),\n        std::nullopt, // No mapping specified in this example\n        [env, loc](OpBuilder &nestedBuilder, Location nestedLoc,\n              ValueRange loopIndices) {\n          Value ivN = loopIndices[0];  // Index for the first dimension N\n          Value ivOH = loopIndices[1]; // Index for the second dimension OH\n          Value ivOW = loopIndices[2]; // Index for the third dimension OW\n          DepthwiseEnv e1 = env;\n          // OC\n          scf::ForOp::create(\n              nestedBuilder, nestedLoc, e1.c0, e1.applyOC, e1.vecSizeValue,\n              ValueRange(),\n              [e1, ivN, ivOH, ivOW](OpBuilder &builder, Location loc,\n                                    Value ivOC, ValueRange iargs) {\n                Value tVec = vector::LoadOp::create(\n                    builder, loc, e1.vecTy, e1.output,\n                    ValueRange(ivN, ivOH, ivOW, ivOC));\n\n                DepthwiseEnv e2 = e1;\n                Value vN = ivN;\n                Value vOH = ivOH;\n                Value vOW = ivOW;\n\n                // FH\n                auto forOp = scf::ForOp::create(\n                    builder, loc, e2.c0, e2.FH, e2.c1, ValueRange(tVec),\n                    [e2, vN, vOH, vOW, ivOC](OpBuilder &builder, Location loc,\n                                             Value ivFH, ValueRange iargs) {\n                      Value rowInput = affine::AffineApplyOp::create(\n                          builder, loc,\n                          AffineMap::get(\n                              2, 0, e2.d0 * e2.strHeight + e2.d1 * e2.dilHeight),\n                          ValueRange(vOH, ivFH));\n                      Value rowFilter = ivFH;\n\n                      DepthwiseEnv e3 = e2;\n                      Value wN = vN;\n                      Value wOW = vOW;\n                      Value wOC = ivOC;\n\n                      // FW\n                      auto forOp = scf::ForOp::create(\n                          builder, loc, e3.c0, e3.FW, e3.c1,\n                          ValueRange(iargs[0]),\n                          [e3, wN, wOW, wOC, rowInput,\n                           rowFilter](OpBuilder &builder, Location loc,\n                                      Value ivFW, ValueRange iargs) {\n                            Value columnInput = affine::AffineApplyOp::create(\n                                builder, loc,\n                                AffineMap::get(2, 0,\n                                               e3.d0 * e3.strWidth +\n                                                   e3.d1 * e3.dilWidth),\n                                ValueRange(wOW, ivFW));\n                            Value columnFilter = affine::AffineApplyOp::create(\n                                builder, loc, AffineMap::get(1, 0, e3.d0),\n                                ivFW);\n                            Value iVec = vector::LoadOp::create(\n                                builder, loc, e3.vecTy, e3.input,\n                                ValueRange(wN, rowInput, columnInput, wOC));\n                            Value fVec = vector::LoadOp::create(\n                                builder, loc, e3.vecTy, e3.filter,\n                                ValueRange(rowFilter, columnFilter, wOC));\n                            Value tVecNext;\n                            if (isa<IntegerType>(e3.elemTy)) {\n                              Value mulVec = arith::MulIOp::create(\n                                  builder, loc, iVec, fVec);\n                              tVecNext = arith::AddIOp::create(\n                                  builder, loc, mulVec, iargs[0]);\n                            } else {\n                              tVecNext = vector::FMAOp::create(\n                                  builder, loc, e3.vecTy, iVec, fVec, iargs[0]);\n                            }\n\n                            scf::YieldOp::create(builder, loc,\n                                                 ValueRange(tVecNext));\n                          });\n                      scf::YieldOp::create(builder, loc,\n                                           ValueRange(forOp.getResult(0)));\n                    });\n                vector::StoreOp::create(builder, loc, forOp.getResult(0),\n                                        e1.output,\n                                        ValueRange(ivN, ivOH, ivOW, ivOC));\n\n                scf::YieldOp::create(builder, loc, ValueRange());\n              });\n\n          // applyOC\n          Value condition = arith::CmpIOp::create(\n              nestedBuilder, loc, arith::CmpIPredicate::sgt, e1.tailLength,\n              e1.c0);\n          scf::IfOp::create(\n              nestedBuilder, loc, condition,\n              [e1, ivN, ivOH, ivOW](OpBuilder &builder, Location loc) {\n                Value tVec = vector::MaskedLoadOp::create(\n                    builder, loc, e1.vecTy, e1.output,\n                    ValueRange(ivN, ivOH, ivOW, e1.applyOC), e1.maskVector,\n                    e1.zeroElementTypeVec);\n\n                DepthwiseEnv e2 = e1;\n                Value tN = ivN;\n                Value tOH = ivOH;\n                Value tOW = ivOW;\n\n                // FH\n                auto forOp = scf::ForOp::create(\n                    builder, loc, e2.c0, e2.FH, e2.c1, ValueRange(tVec),\n                    [e2, tN, tOH, tOW](OpBuilder &builder, Location loc,\n                                       Value ivFH, ValueRange iargs) {\n                      Value rowInput = affine::AffineApplyOp::create(\n                          builder, loc,\n                          AffineMap::get(\n                              2, 0, e2.d0 * e2.strHeight + e2.d1 * e2.dilHeight),\n                          ValueRange(tOH, ivFH));\n                      Value rowFilter = ivFH;\n\n                      DepthwiseEnv e3 = e2;\n                      Value uN = tN;\n                      Value uOW = tOW;\n\n                      // FW\n                      auto forOp = scf::ForOp::create(\n                          builder, loc, e3.c0, e3.FW, e3.c1,\n                          ValueRange(iargs[0]),\n                          [e3, uN, uOW, rowInput,\n                           rowFilter](OpBuilder &builder, Location loc,\n                                      Value ivFW, ValueRange iargs) {\n                            Value columnInput = affine::AffineApplyOp::create(\n                                builder, loc,\n                                AffineMap::get(2, 0,\n                                               e3.d0 * e3.strWidth +\n                                                   e3.d1 * e3.dilWidth),\n                                ValueRange(uOW, ivFW));\n                            Value columnFilter = affine::AffineApplyOp::create(\n                                builder, loc, AffineMap::get(1, 0, e3.d0),\n                                ivFW);\n                            Value iVec = vector::MaskedLoadOp::create(\n                                builder, loc, e3.vecTy, e3.input,\n                                ValueRange(uN, rowInput, columnInput,\n                                           e3.applyOC),\n                                e3.maskVector, e3.zeroElementTypeVec);\n                            Value fVec = vector::MaskedLoadOp::create(\n                                builder, loc, e3.vecTy, e3.filter,\n                                ValueRange(rowFilter, columnFilter,\n                                           e3.applyOC),\n                                e3.maskVector, e3.zeroElementTypeVec);\n                            Value tVecNext;\n                            if (isa<IntegerType>(e3.elemTy)) {\n                              Value mulVec = arith::MulIOp::create(\n                                  builder, loc, iVec, fVec);\n                              tVecNext = arith::AddIOp::create(\n                                  builder, loc, mulVec, iargs[0]);\n                            } else {\n                              tVecNext = vector::FMAOp::create(\n                                  builder, loc, e3.vecTy, iVec, fVec, iargs[0]);\n                            }\n\n                            scf::YieldOp::create(builder, loc,\n                                                 ValueRange(tVecNext));\n                          });\n                      scf::YieldOp::create(builder, loc,\n                                           ValueRange(forOp.getResult(0)));\n                    });\n                vector::MaskedStoreOp::create(\n                    builder, loc, e1.output,\n                    ValueRange(ivN, ivOH, ivOW, e1.applyOC), e1.maskVector,\n                    forOp.getResult(0));\n                scf::YieldOp::create(builder, loc, ValueRange());\n              });\n\n          scf::InParallelOp::create(nestedBuilder, nestedLoc);\n        });\n    // clang format on'

def rule_nested_builder(text, ns):
    """把 scf builder 的多层 lambda 改成逐层拷贝环境的写法。"""
    if 'DepthwiseConv2DNhwcHwcOp' not in text:
        return text
    if 'struct DepthwiseEnv' not in text:
        marker = 'namespace ' + ns + ' {'
        i = text.find(marker)
        if i >= 0:
            j = text.index('{', i) + 1
            text = text[:j] + '\n' + DEPTHWISE_ENV + text[j:]
    start = text.find('    // clang format off')
    end = text.find('    // clang format on')
    if start >= 0 and end >= 0:
        end += len('    // clang format on\n')
        text = text[:start] + DEPTHWISE_BLOCK + text[end:]
    return text


# === MODEL BEGIN ===
MODEL = {
    'llvm/ADT/ArrayRef.h': r'''#ifndef FC_LLVM_ADT_ARRAYREF_H
#define FC_LLVM_ADT_ARRAYREF_H

#include <initializer_list>

#include "llvm/ADT/SmallVector.h"

namespace llvm {

template <class T>
class ArrayRef {
public:
  int reserved;

  ArrayRef();
  ArrayRef(const SmallVector<T> &other);
  ArrayRef(std::initializer_list<T> values);

  const T *begin() const;
  const T *end() const;

  bool empty() const;
  fc_size_t size() const;

  const T &back() const;
  const T &operator[](fc_size_t index) const;

  bool operator!=(const ArrayRef<T> &other) const;
};

} // namespace llvm
#endif
''',
    'llvm/ADT/FunctionRef.h': r'''#ifndef FC_LLVM_ADT_FUNCTIONREF_H
#define FC_LLVM_ADT_FUNCTIONREF_H

/* Stand-in for llvm::function_ref<Ret(Params...)>: a cheap, non-owning handle to
 * any callable with the given signature (lambda, capturing lambda or plain
 * function pointer).  The upstream single pack-based partial specialisation is
 * spelled out per arity here because Frama-Clang 0.0.20 cannot instantiate the
 * pack form; the interface is the same. */
namespace llvm {

template <typename Fn>
class function_ref;

template <typename Ret>
class function_ref<Ret()> {
public:
  int reserved;

  function_ref() {}

  template <typename Callable>
  function_ref(Callable callable) {}

  Ret operator()() const;
};

template <typename Ret, typename A1>
class function_ref<Ret(A1)> {
public:
  int reserved;

  function_ref() {}

  template <typename Callable>
  function_ref(Callable callable) {}

  Ret operator()(A1 a1) const;
};

template <typename Ret, typename A1, typename A2>
class function_ref<Ret(A1, A2)> {
public:
  int reserved;

  function_ref() {}

  template <typename Callable>
  function_ref(Callable callable) {}

  Ret operator()(A1 a1, A2 a2) const;
};

template <typename Ret, typename A1, typename A2, typename A3>
class function_ref<Ret(A1, A2, A3)> {
public:
  int reserved;

  function_ref() {}

  template <typename Callable>
  function_ref(Callable callable) {}

  Ret operator()(A1 a1, A2 a2, A3 a3) const;
};

template <typename Ret, typename A1, typename A2, typename A3, typename A4>
class function_ref<Ret(A1, A2, A3, A4)> {
public:
  int reserved;

  function_ref() {}

  template <typename Callable>
  function_ref(Callable callable) {}

  Ret operator()(A1 a1, A2 a2, A3 a3, A4 a4) const;
};

template <typename Ret, typename A1, typename A2, typename A3, typename A4,
          typename A5>
class function_ref<Ret(A1, A2, A3, A4, A5)> {
public:
  int reserved;

  function_ref() {}

  template <typename Callable>
  function_ref(Callable callable) {}

  Ret operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const;
};

} // namespace llvm
#endif
''',
    'llvm/ADT/SmallVector.h': r'''#ifndef FC_LLVM_ADT_SMALLVECTOR_H
#define FC_LLVM_ADT_SMALLVECTOR_H

#include <initializer_list>

#include "llvm/Support/Compiler.h"

namespace llvm {

template <class T> class ArrayRef;

/* N is the inline capacity in the real class; the model only needs the
 * spelling So that `SmallVector<T, 3>` parses. */
template <class T, unsigned N = 0>
class SmallVector {
public:
  typedef T *iterator;
  typedef const T *const_iterator;

  int reserved;

  SmallVector();
  SmallVector(const SmallVector &other);
  SmallVector(std::initializer_list<T> values);
  SmallVector &operator=(const SmallVector &other);

  template <class It>
  SmallVector(It first, It last);

  template <class U>
  SmallVector(const ArrayRef<U> &ref);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  bool empty() const;
  fc_size_t size() const;

  T &back();
  const T &back() const;

  T &operator[](fc_size_t index);
  const T &operator[](fc_size_t index) const;

  void reserve(fc_size_t count);
  void push_back(const T &value);
};

} // namespace llvm
#endif
''',
    'llvm/ADT/STLExtras.h': r'''#ifndef FC_LLVM_ADT_STLEEXTRAS_H
#define FC_LLVM_ADT_STLEEXTRAS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

template <class T>
class EnumerateItem {
public:
  fc_size_t first;
  T second;
};

template <class T>
class EnumerateIterator {
public:
  EnumerateIterator(const T *cursor, fc_size_t index)
      : cursor(cursor), index(index) {}

  bool operator!=(const EnumerateIterator &other) const {
    return cursor != other.cursor;
  }
  void operator++() {
    ++cursor;
    ++index;
  }
  EnumerateItem<T> operator*() const {
    EnumerateItem<T> item;
    item.first = index;
    item.second = *cursor;
    return item;
  }

private:
  const T *cursor;
  fc_size_t index;
};

template <class T>
class EnumerateRange {
public:
  EnumerateRange(const T *first, const T *last) : first(first), last(last) {}

  EnumerateIterator<T> begin() const { return EnumerateIterator<T>(first, 0); }
  EnumerateIterator<T> end() const { return EnumerateIterator<T>(last, 0); }

private:
  const T *first;
  const T *last;
};

template <class T>
inline EnumerateRange<T> enumerate(ArrayRef<T> range) {
  return EnumerateRange<T>(range.begin(), range.end());
}

/* `llvm::make_early_inc_range(x)`: the model hands the range back unchanged,
 * enough to type-check walk-while-erasing loops. */
template <class T>
T make_early_inc_range(T range) {
  return range;
}

} // namespace llvm
#endif
''',
    'llvm/ADT/StringRef.h': r'''#ifndef FC_LLVM_ADT_STRINGREF_H
#define FC_LLVM_ADT_STRINGREF_H

#include "llvm/Support/Compiler.h"

namespace llvm {

class StringRef {
public:
  int reserved;

  StringRef();
  StringRef(const char *text);

  const char *data() const;
};

} // namespace llvm
#endif
''',
    'llvm/Support/Casting.h': r'''#ifndef FC_LLVM_SUPPORT_CASTING_H
#define FC_LLVM_SUPPORT_CASTING_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"

/* The passes spell casts as `llvm::dyn_cast<...>(...)`; forward to the mlir
 * overloads.  In the real tree this header defines the generic casting
 * machinery instead. */
namespace llvm {

template <class T>
T dyn_cast(mlir::Operation *operation) {
  return mlir::dyn_cast<T>(operation);
}

template <class T>
T dyn_cast(mlir::Attribute attribute) {
  return mlir::dyn_cast<T>(attribute);
}

template <class T>
T dyn_cast(mlir::Value value) {
  return mlir::dyn_cast<T>(value);
}

template <class T>
T dyn_cast(mlir::Type type) {
  return mlir::dyn_cast<T>(type);
}

} // namespace llvm
#endif
''',
    'llvm/Support/CommandLine.h': r'''#ifndef FC_LLVM_SUPPORT_COMMANDLINE_H
#define FC_LLVM_SUPPORT_COMMANDLINE_H

#include "llvm/Support/Compiler.h"

namespace llvm {
namespace cl {

class desc {
public:
  int reserved;

  explicit desc(const char *text);
};

/* `int` keeps both `cl::init(true)` and `cl::init(16)` unambiguous. */
class init {
public:
  int reserved;

  explicit init(int value);
};

} // namespace cl
} // namespace llvm
#endif
''',
    'llvm/Support/Compiler.h': r'''#ifndef FC_LLVM_SUPPORT_COMPILER_H
#define FC_LLVM_SUPPORT_COMPILER_H

/* Minimal compiler/type shim: the model only needs fixed-width integers. */
typedef __INT64_TYPE__ int64_t;
typedef __SIZE_TYPE__ fc_size_t;
typedef __SIZE_TYPE__ size_t;
#endif
''',
    'llvm/Support/ErrorHandling.h': r'''#ifndef FC_LLVM_SUPPORT_ERRORHANDLING_H
#define FC_LLVM_SUPPORT_ERRORHANDLING_H

namespace llvm {

void report_fatal_error(const char *reason, bool gen_crash_diag = true);

} // namespace llvm

#define llvm_unreachable(msg) ::llvm::report_fatal_error(msg, true)
#endif
''',
    'mlir/Conversion/LLVMCommon/ConversionTarget.h': r'''#ifndef FC_MLIR_CONVERSION_LLVMCOMMON_CONVERSIONTARGET_H
#define FC_MLIR_CONVERSION_LLVMCOMMON_CONVERSIONTARGET_H

#include "mlir/IR/MLIRContext.h"
#include "mlir/Transforms/DialectConversion.h"

namespace mlir {

class LLVMConversionTarget {
public:
  int reserved;

  explicit LLVMConversionTarget(MLIRContext &context);
};

} // namespace mlir
#endif
''',
    'mlir/Conversion/LLVMCommon/TypeConverter.h': r'''#ifndef FC_MLIR_CONVERSION_LLVMCOMMON_TYPECONVERTER_H
#define FC_MLIR_CONVERSION_LLVMCOMMON_TYPECONVERTER_H

#include "mlir/IR/MLIRContext.h"

namespace mlir {

class LLVMTypeConverter {
public:
  int reserved;

  explicit LLVMTypeConverter(MLIRContext *context);
};

} // namespace mlir
#endif
''',
    'mlir/Dialect/Affine/IR/AffineOps.h': r'''#ifndef FC_MLIR_DIALECT_AFFINE_IR_AFFINEOPS_H
#define FC_MLIR_DIALECT_AFFINE_IR_AFFINEOPS_H

#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {
namespace affine {

class AffineDialect {
public:
  int reserved;
};

class AffineApplyOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, AffineMap map,
                      ValueRange operands);

  Value getResult();

  static bool classofOp(Operation *operation);
  static AffineApplyOp castFrom(Operation *operation);
};

} // namespace affine
} // namespace mlir
#endif
''',
    'mlir/Dialect/Arith/IR/Arith.h': r'''#ifndef FC_MLIR_DIALECT_ARITH_IR_ARITH_H
#define FC_MLIR_DIALECT_ARITH_IR_ARITH_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir {
namespace arith {

class ArithDialect {
public:
  int reserved;
};

enum class CmpIPredicate { eq, ne, slt, sle, sgt, sge, ult, ule, ugt, uge };

class ConstantOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, Attribute value);
  static Value create(OpBuilder &builder, Location location,
                      OpFoldResult value);

  static bool classofOp(Operation *operation);
};

class MulIOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, Value lhs,
                      Value rhs);

  static bool classofOp(Operation *operation);
};

class AddIOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, Value lhs,
                      Value rhs);

  static bool classofOp(Operation *operation);
};

class CmpIOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location,
                      CmpIPredicate predicate, Value lhs, Value rhs);

  static bool classofOp(Operation *operation);
};

} // namespace arith
} // namespace mlir
#endif
''',
    'mlir/Dialect/Bufferization/Transforms/Bufferize.h': r'''#ifndef FC_MLIR_DIALECT_BUFFERIZATION_TRANSFORMS_BUFFERIZE_H
#define FC_MLIR_DIALECT_BUFFERIZATION_TRANSFORMS_BUFFERIZE_H

#include "mlir/IR/Operation.h"
#endif
''',
    'mlir/Dialect/Func/IR/FuncOps.h': r'''#ifndef FC_MLIR_DIALECT_FUNC_IR_FUNCOPS_H
#define FC_MLIR_DIALECT_FUNC_IR_FUNCOPS_H

#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"

namespace mlir {
namespace func {

class FuncOp : public Operation {
public:
  FuncOp();

  MLIRContext *getContext() const;
  Operation *getOperation();
  Operation *operator->();
  Block *begin();
  Block *end();

  bool isDeclaration() const;
  bool isExternal() const;
  Region &getBody();
  unsigned getNumArguments() const;
  BlockArgument getArgument(unsigned index);
  FunctionType getFunctionType() const;
  void setType(FunctionType type);
  llvm::ArrayRef<Type> getArgumentTypes() const;

  template <class F>
  void walk(F function) {}

  static bool classofOp(Operation *operation);
  static FuncOp castFrom(Operation *operation);
};

class ReturnOp : public Operation {
public:
  ReturnOp();

  Operation *getOperation();
  Operation *operator->();
  operator Operation *();
  explicit operator bool() const;

  static bool classofOp(Operation *operation) {
    return operation != 0 && operation->getKind() == 1;
  }
  static ReturnOp castFrom(Operation *operation);
};

} // namespace func
} // namespace mlir
#endif
''',
    'mlir/Dialect/GPU/IR/GPUDialect.h': r'''#ifndef FC_MLIR_DIALECT_GPU_IR_GPUDIALECT_H
#define FC_MLIR_DIALECT_GPU_IR_GPUDIALECT_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {
namespace gpu {

class GPUDialect {
public:
  int reserved;
};

enum class AddressSpace { Global = 1, Workgroup = 3, Private = 5 };

class AddressSpaceAttr : public Attribute {
public:
  AddressSpaceAttr();
  AddressSpaceAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);
  static AddressSpaceAttr castFrom(Attribute attribute);

  AddressSpace getValue() const;
};

class LaunchOp {
public:
  int reserved;

  LaunchOp();

  Operation *getOperation();
  Operation *operator->();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static LaunchOp castFrom(Operation *operation);
};

class AllocOp {
public:
  int reserved;

  AllocOp();

  static AllocOp create(OpBuilder &builder, Location location,
                        TypeRange resultTypes, ValueRange dynamicSizes);

  Operation *getOperation();
  Location getLoc();
  Value getResult(int64_t index);
  Operation *operator->();
  operator Operation *();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static AllocOp castFrom(Operation *operation);
};

/* gpu.memcpy: destination and source are swapped w.r.t. memref.copy. */
class MemcpyOp {
public:
  int reserved;

  MemcpyOp();

  static MemcpyOp create(OpBuilder &builder, Location location,
                         TypeRange resultTypes, ValueRange operands, Value dst,
                         Value src);

  Operation *getOperation();
  Location getLoc();
  Value getResult(int64_t index);
  Operation *operator->();
  operator Operation *();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static MemcpyOp castFrom(Operation *operation);
};

class DeallocOp {
public:
  int reserved;

  DeallocOp();

  static DeallocOp create(OpBuilder &builder, Location location,
                          TypeRange resultTypes, ValueRange operands,
                          Value memref);

  Operation *getOperation();
  Location getLoc();
  Operation *operator->();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static DeallocOp castFrom(Operation *operation);
};

} // namespace gpu
} // namespace mlir
#endif
''',
    'mlir/Dialect/Linalg/IR/Linalg.h': r'''#ifndef FC_MLIR_DIALECT_LINALG_IR_LINALG_H
#define FC_MLIR_DIALECT_LINALG_IR_LINALG_H

#include "mlir/Dialect/Linalg/IR/LinalgOps.h"

namespace mlir {
namespace linalg {

class LinalgDialect {
public:
  int reserved;
};

} // namespace linalg
} // namespace mlir
#endif
''',
    'mlir/Dialect/Linalg/IR/LinalgOps.h': r'''#ifndef FC_MLIR_DIALECT_LINALG_IR_LINALGOPS_H
#define FC_MLIR_DIALECT_LINALG_IR_LINALGOPS_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"

namespace mlir {
namespace linalg {

class FillOp {
public:
  int reserved;

  static bool classofOp(Operation *operation);
};

/* Depthwise convolution the pass legalises: NHWC input/output with an HWC
 * filter, i.e. one FH x FW kernel per channel. */
class DepthwiseConv2DNhwcHwcOp {
public:
  int reserved;

  DepthwiseConv2DNhwcHwcOp();

  static StringRef getOperationName();

  DenseIntElementsAttr getStrides();
  DenseIntElementsAttr getDilations();

  Operation *getOperation();

  static bool classofOp(Operation *operation);
  static DepthwiseConv2DNhwcHwcOp castFrom(Operation *operation);
};

} // namespace linalg
} // namespace mlir
#endif
''',
    'mlir/Dialect/Linalg/Transforms/Transforms.h': r'''#ifndef FC_MLIR_DIALECT_LINALG_TRANSFORMS_TRANSFORMS_H
#define FC_MLIR_DIALECT_LINALG_TRANSFORMS_TRANSFORMS_H

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/DialectConversion.h"
#endif
''',
    'mlir/Dialect/LLVMIR/LLVMDialect.h': r'''#ifndef FC_MLIR_DIALECT_LLVMIR_LLVMDIALECT_H
#define FC_MLIR_DIALECT_LLVMIR_LLVMDIALECT_H

#include "mlir/IR/Operation.h"

namespace mlir {
namespace LLVM {

class LLVMDialect {
public:
  int reserved;
};

} // namespace LLVM
} // namespace mlir
#endif
''',
    'mlir/Dialect/MemRef/IR/MemRef.h': r'''#ifndef FC_MLIR_DIALECT_MEMREF_IR_MEMREF_H
#define FC_MLIR_DIALECT_MEMREF_IR_MEMREF_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"

namespace mlir {
namespace memref {

class MemRefDialect {
public:
  int reserved;
};

class DimOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, Value source,
                      int64_t index);

  static bool classofOp(Operation *operation);
  static DimOp castFrom(Operation *operation);
};

class CopyOp {
public:
  int reserved;

  CopyOp();

  Value getSource();
  Value getTarget();
  Value getOperand(int64_t index);
  Location getLoc();
  Operation *getOperation();
  Operation *operator->();
  operator Operation *();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static CopyOp castFrom(Operation *operation);
};

class AllocOp {
public:
  int reserved;

  AllocOp();

  static AllocOp create(OpBuilder &builder, Location location,
                        MemRefType resultType);

  Operation *getOperation();
  Location getLoc();
  Value getResult(int64_t index);
  Operation *operator->();
  operator Operation *();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static AllocOp castFrom(Operation *operation);
};

class DeallocOp {
public:
  int reserved;

  DeallocOp();

  Operation *getOperation();
  Location getLoc();
  Operation *operator->();
  operator Operation *();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static DeallocOp castFrom(Operation *operation);
};

class GetGlobalOp {
public:
  int reserved;

  GetGlobalOp();

  Operation *getOperation();
  Location getLoc();
  Value getResult(int64_t index);
  Operation *operator->();
  operator Operation *();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static GetGlobalOp castFrom(Operation *operation);
};

class CastOp {
public:
  int reserved;

  CastOp();

  static CastOp create(OpBuilder &builder, Location location,
                       MemRefType resultType, Value source);

  Value getSource();
  operator Value() const;
  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static CastOp castFrom(Operation *operation);
};

class ReinterpretCastOp {
public:
  int reserved;

  ReinterpretCastOp();

  static ReinterpretCastOp create(OpBuilder &builder, Location location,
                                  MemRefType resultType, Value source,
                                  OpFoldResult offset,
                                  llvm::SmallVector<OpFoldResult> sizes,
                                  llvm::SmallVector<OpFoldResult> strides);

  operator Value();

  Value getSource();

  operator Operation *();
  Operation *getOperation();
  Location getLoc();
  Value getOperand(int64_t index);
  OpResult getResult();
  void erase();

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static ReinterpretCastOp castFrom(Operation *operation);
};

class ExtractStridedMetadataOp {
public:
  int reserved;

  static ExtractStridedMetadataOp create(OpBuilder &builder, Location location,
                                         Value source);

  llvm::SmallVector<OpFoldResult> getSizes() const;
  llvm::SmallVector<OpFoldResult> getStrides() const;
  OpFoldResult getOffset() const;
  Value getBaseBuffer() const;
  Value getSource() const;

  bool operator!() const;
  explicit operator bool() const;

  static bool classofOp(Operation *operation);
  static ExtractStridedMetadataOp castFrom(Operation *operation);
};

} // namespace memref
} // namespace mlir
#endif
''',
    'mlir/Dialect/SCF/IR/SCF.h': r'''#ifndef FC_MLIR_DIALECT_SCF_IR_SCF_H
#define FC_MLIR_DIALECT_SCF_IR_SCF_H

#include "llvm/ADT/FunctionRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {
namespace scf {

class SCFDialect {
public:
  int reserved;
};

class YieldOp {
public:
  int reserved;

  static YieldOp create(OpBuilder &builder, Location location,
                        ValueRange values);

  static bool classofOp(Operation *operation);
};

/* Structured for.  The body builder is a `function_ref`, exactly like the real
 * op: both plain functions and (capturing) lambdas bind to it. */
class ForOp {
public:
  int reserved;

  Value getInductionVar();
  Value getResult(int64_t index);

  static ForOp create(
      OpBuilder &builder, Location location, Value lowerBound, Value upperBound,
      Value step, ValueRange iterArgs,
      llvm::function_ref<void(OpBuilder &, Location, Value, ValueRange)> body);

  static bool classofOp(Operation *operation);
  static ForOp castFrom(Operation *operation);
};

class ForallOp {
public:
  int reserved;

  static ForallOp create(
      OpBuilder &builder, Location location,
      SmallVector<OpFoldResult, 3> lowerBounds, ValueRange iterArgs,
      std::nullopt_t mapping,
      llvm::function_ref<void(OpBuilder &, Location, ValueRange)> body);

  static bool classofOp(Operation *operation);
  static ForallOp castFrom(Operation *operation);
};

class IfOp {
public:
  int reserved;

  static IfOp create(
      OpBuilder &builder, Location location, Value condition,
      llvm::function_ref<void(OpBuilder &, Location)> thenBody);

  static bool classofOp(Operation *operation);
  static IfOp castFrom(Operation *operation);
};

class InParallelOp {
public:
  int reserved;

  static InParallelOp create(OpBuilder &builder, Location location);

  static bool classofOp(Operation *operation);
};

} // namespace scf
} // namespace mlir
#endif
''',
    'mlir/Dialect/Vector/IR/VectorOps.h': r'''#ifndef FC_MLIR_DIALECT_VECTOR_IR_VECTOROPS_H
#define FC_MLIR_DIALECT_VECTOR_IR_VECTOROPS_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/ValueRange.h"

namespace mlir {

class VectorDialect {
public:
  int reserved;
};

namespace vector {

class LoadOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, VectorType type,
                      Value base, ValueRange indices);

  static bool classofOp(Operation *operation);
  static LoadOp castFrom(Operation *operation);
};

class StoreOp {
public:
  int reserved;

  static void create(OpBuilder &builder, Location location, Value value,
                     Value base, ValueRange indices);

  static bool classofOp(Operation *operation);
  static StoreOp castFrom(Operation *operation);
};

class BroadcastOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, VectorType type,
                      Value source);

  static bool classofOp(Operation *operation);
};

/* `create_mask`: produces the vector<...x i1> tail mask. */
class CreateMaskOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, VectorType type,
                      ValueRange lengths);

  static bool classofOp(Operation *operation);
};

class MaskedLoadOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, VectorType type,
                      Value base, ValueRange indices, Value mask,
                      Value passThru);

  static bool classofOp(Operation *operation);
};

class MaskedStoreOp {
public:
  int reserved;

  static void create(OpBuilder &builder, Location location, Value base,
                     ValueRange indices, Value mask, Value value);

  static bool classofOp(Operation *operation);
};

class FMAOp {
public:
  int reserved;

  static Value create(OpBuilder &builder, Location location, VectorType type,
                      Value lhs, Value rhs, Value acc);

  static bool classofOp(Operation *operation);
};

} // namespace vector
} // namespace mlir
#endif
''',
    'mlir/Interfaces/CallInterfaces.h': r'''#ifndef FC_MLIR_INTERFACES_CALLINTERFACES_H
#define FC_MLIR_INTERFACES_CALLINTERFACES_H

#include "mlir/IR/Operation.h"

namespace mlir {

class CallOpInterface {
public:
  int reserved;

  static bool classofOp(Operation *operation) { return false; }
};

} // namespace mlir
#endif
''',
    'mlir/Interfaces/ControlFlowInterfaces.h': r'''#ifndef FC_MLIR_INTERFACES_CONTROLFLOWINTERFACES_H
#define FC_MLIR_INTERFACES_CONTROLFLOWINTERFACES_H

#include "mlir/IR/Operation.h"

namespace mlir {

class BranchOpInterface {
public:
  int reserved;

  static bool classofOp(Operation *operation) { return false; }
};

class RegionBranchOpInterface {
public:
  int reserved;

  static bool classofOp(Operation *operation) { return false; }
};

} // namespace mlir
#endif
''',
    'mlir/IR/AffineExpr.h': r'''#ifndef FC_MLIR_IR_AFFINEEXPR_H
#define FC_MLIR_IR_AFFINEEXPR_H

#include "llvm/Support/Compiler.h"

namespace mlir {

/* Affine expression handle.  Only the arithmetic the passes spell out is
 * modelled: scaling by a constant, addition, modulo and floor division. */
class AffineExpr {
public:
  int reserved;

  AffineExpr();

  AffineExpr operator*(int64_t rhs) const;
  AffineExpr operator+(const AffineExpr &rhs) const;
  AffineExpr operator%(int64_t rhs) const;
  AffineExpr floorDiv(int64_t rhs) const;
};

} // namespace mlir
#endif
''',
    'mlir/IR/AffineMap.h': r'''#ifndef FC_MLIR_IR_AFFINEMAP_H
#define FC_MLIR_IR_AFFINEMAP_H

#include "mlir/IR/AffineExpr.h"

namespace mlir {

class AffineMap {
public:
  int reserved;

  AffineMap();

  static AffineMap get(int64_t dimCount, int64_t symbolCount, AffineExpr expr);
};

} // namespace mlir
#endif
''',
    'mlir/IR/Attributes.h': r'''#ifndef FC_MLIR_IR_ATTRIBUTES_H
#define FC_MLIR_IR_ATTRIBUTES_H

#include "mlir/IR/Types.h"

namespace mlir {

enum AttributeKind { AttrNone = 0, AttrStridedLayout = 1, AttrMemRefLayout = 2,
                     AttrDenseIntElements = 3 };

class Attribute {
public:
  Attribute() : kind(AttrNone) {}
  explicit Attribute(int k) : kind(k) {}

  bool operator!() const { return kind == AttrNone; }
  explicit operator bool() const { return kind != AttrNone; }
  int getKind() const { return kind; }

private:
  int kind;
};

template <class T>
bool isa(Attribute attribute) {
  return T::classof(attribute);
}

template <class T>
T dyn_cast(Attribute attribute) {
  if (T::classof(attribute))
    return T::castFrom(attribute);
  return T();
}

template <class T>
T cast(Attribute attribute) {
  return T::castFrom(attribute);
}

} // namespace mlir
#endif
''',
    'mlir/IR/Block.h': r'''#ifndef FC_MLIR_IR_BLOCK_H
#define FC_MLIR_IR_BLOCK_H

#include "mlir/IR/Value.h"

namespace mlir {

class Operation;

using BlockArgListType = llvm::ArrayRef<BlockArgument>;

class Block {
public:
  int reserved;

  Block();

  Operation *getParentOp() const;
  BlockArgListType getArguments();
};

} // namespace mlir
#endif
''',
    'mlir/IR/Builders.h': r'''#ifndef FC_MLIR_IR_BUILDERS_H
#define FC_MLIR_IR_BUILDERS_H

#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Operation.h"

namespace mlir {

class OpBuilder {
public:
  int reserved;

  explicit OpBuilder(MLIRContext *context);

  void setInsertionPointAfter(Operation *operation);
  void setInsertionPointToStart(Block *block);

  template <class T>
  void setInsertionPoint(T op) {}

  IntegerAttr getIndexAttr(int64_t value);
  Attribute getZeroAttr(Type type);
  Type getI1Type();

  AffineExpr getAffineDimExpr(unsigned position);
  AffineExpr getAffineSymbolExpr(unsigned position);

  Location getUnknownLoc();
  FunctionType getFunctionType(llvm::ArrayRef<Type> inputs,
                               llvm::ArrayRef<Type> results);
};

} // namespace mlir
#endif
''',
    'mlir/IR/BuiltinAttributes.h': r'''#ifndef FC_MLIR_IR_BUILTINATTRIBUTES_H
#define FC_MLIR_IR_BUILTINATTRIBUTES_H

#include "mlir/IR/Attributes.h"

namespace mlir {

class StridedLayoutAttr : public Attribute {
public:
  StridedLayoutAttr();
  StridedLayoutAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute) {
    return attribute.getKind() == AttrStridedLayout;
  }
  static StridedLayoutAttr castFrom(Attribute attribute) {
    return StridedLayoutAttr(Attribute(AttrStridedLayout));
  }

  llvm::ArrayRef<int64_t> getStrides() const;
  int64_t getOffset() const;

  static StridedLayoutAttr get(MLIRContext *context, int64_t offset,
                               llvm::ArrayRef<int64_t> strides);
};

/* DenseI64ArrayAttr: 官方接口是 `asArrayRef<T>()`（不是 getValues）。 */
class DenseI64ArrayAttr : public Attribute {
public:
  DenseI64ArrayAttr();
  DenseI64ArrayAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);
  static DenseI64ArrayAttr castFrom(Attribute attribute);

  template <class T>
  llvm::ArrayRef<T> asArrayRef() const;

  int64_t size() const;
};

/* DenseIntElementsAttr: 官方 `getValues<T>()`；linalg 的 strides/dilations
 * 访问器返回的就是它。 */
class DenseIntElementsAttr : public Attribute {
public:
  DenseIntElementsAttr();
  DenseIntElementsAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute) {
    return attribute.getKind() == AttrDenseIntElements;
  }
  static DenseIntElementsAttr castFrom(Attribute attribute) {
    return DenseIntElementsAttr(Attribute(AttrDenseIntElements));
  }

  template <class T>
  llvm::ArrayRef<T> getValues() const;
};

/* Integer attribute: `getInt()` is what the pass compares against 0. */
class IntegerAttr : public Attribute {
public:
  IntegerAttr();
  IntegerAttr(const Attribute &other) : Attribute(other) {}

  static bool classof(Attribute attribute);
  static IntegerAttr castFrom(Attribute attribute);

  int64_t getInt() const;
};

class MemRefLayoutAttrInterface : public Attribute {
public:
  MemRefLayoutAttrInterface();
};

} // namespace mlir
#endif
''',
    'mlir/IR/BuiltinOps.h': r'''#ifndef FC_MLIR_IR_BUILTINOPS_H
#define FC_MLIR_IR_BUILTINOPS_H

#include "mlir/IR/Operation.h"

namespace mlir {

class ModuleOp {
public:
  int reserved;

  ModuleOp();
};

} // namespace mlir
#endif
''',
    'mlir/IR/BuiltinTypes.h': r'''#ifndef FC_MLIR_IR_BUILTINTYPES_H
#define FC_MLIR_IR_BUILTINTYPES_H

#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Types.h"

namespace mlir {

class ShapedType : public Type {
public:
  ShapedType();
  ShapedType(const Type &other) : Type(other) {}

  static bool classof(Type type);
  static ShapedType castFrom(Type type);

  static bool isDynamic(int64_t value) { return value < 0; }

  Type getElementType() const;
};

class IntegerType : public Type {
public:
  IntegerType();
  IntegerType(const Type &other) : Type(other) {}

  static bool classof(Type type) { return type.getKind() == TypeInteger; }
  static IntegerType castFrom(Type type) {
    return IntegerType(Type(TypeInteger));
  }
};

class FloatType : public Type {
public:
  FloatType();
  FloatType(const Type &other) : Type(other) {}

  static bool classof(Type type) { return type.getKind() == TypeFloat; }
  static FloatType castFrom(Type type) {
    return FloatType(Type(TypeFloat));
  }
};

/* Vector type handle: `get(count, elem)` and `get(shape, elem)` mirror the
 * two constructors the passes spell out. */
class VectorType : public Type {
public:
  VectorType();
  VectorType(const Type &other) : Type(other) {}

  static bool classof(Type type) { return type.getKind() == TypeVector; }
  static VectorType castFrom(Type type) {
    return VectorType(Type(TypeVector));
  }

  static VectorType get(int64_t numElements, Type elementType);
  static VectorType get(llvm::ArrayRef<int64_t> shape, Type elementType);

  Type getElementType() const;
};

class FunctionType : public Type {
public:
  FunctionType();
  FunctionType(const Type &other) : Type(other) {}

  static bool classof(Type type);
  static FunctionType castFrom(Type type);

  llvm::ArrayRef<Type> getResults() const;
  llvm::ArrayRef<Type> getInputs() const;
};

class MemRefType : public Type {
public:
  MemRefType();
  MemRefType(const Type &other) : Type(other) {}

  static bool classof(Type type) { return type.getKind() == TypeMemRef; }
  static MemRefType castFrom(Type type) {
    return MemRefType(Type(TypeMemRef));
  }

  bool hasRank() const;
  int64_t getRank() const;
  Attribute getLayout() const;
  llvm::ArrayRef<int64_t> getShape() const;
  Type getElementType() const;
  MLIRContext *getContext() const;
  Attribute getMemorySpace() const;

  static MemRefType get(llvm::ArrayRef<int64_t> shape, Type elementType,
                        Attribute layout, Attribute memorySpace);

  /* Real MLIR also accepts an AffineMap for the layout. */
  static MemRefType get(llvm::ArrayRef<int64_t> shape, Type elementType,
                        AffineMap layout, Attribute memorySpace);
};

} // namespace mlir
#endif
''',
    'mlir/IR/Dialect.h': r'''#ifndef FC_MLIR_IR_DIALECT_H
#define FC_MLIR_IR_DIALECT_H

#include "mlir/IR/MLIRContext.h"

namespace mlir {

class Dialect {
public:
  int reserved;

  Dialect();
  virtual ~Dialect();

  MLIRContext *getContext() const;
};

} // namespace mlir
#endif
''',
    'mlir/IR/DialectRegistry.h': r'''#ifndef FC_MLIR_IR_DIALECTREGISTRY_H
#define FC_MLIR_IR_DIALECTREGISTRY_H

#include "mlir/IR/MLIRContext.h"

namespace mlir {

class DialectRegistry {
public:
  int reserved;

  DialectRegistry();

  template <class... Dialects>
  void insert() {}
};

} // namespace mlir
#endif
''',
    'mlir/IR/IntegerSet.h': r'''#ifndef FC_MLIR_IR_INTEGERSET_H
#define FC_MLIR_IR_INTEGERSET_H

namespace mlir {

/* Included by the pass but never used; kept so the include resolves exactly
 * like the real header does. */
class IntegerSet {
public:
  int reserved;

  IntegerSet();
};

} // namespace mlir
#endif
''',
    'mlir/IR/Location.h': r'''#ifndef FC_MLIR_IR_LOCATION_H
#define FC_MLIR_IR_LOCATION_H

#include "mlir/IR/MLIRContext.h"

namespace mlir {

class Location {
public:
  int reserved;
};

} // namespace mlir
#endif
''',
    'mlir/IR/MLIRContext.h': r'''#ifndef FC_MLIR_IR_MLIRCONTEXT_H
#define FC_MLIR_IR_MLIRCONTEXT_H

#include "mlir/Support/LLVM.h"

namespace mlir {

class MLIRContext {
public:
  int reserved;
};

} // namespace mlir
#endif
''',
    'mlir/IR/OpDefinition.h': r'''#ifndef FC_MLIR_IR_OPDEFINITION_H
#define FC_MLIR_IR_OPDEFINITION_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Operation.h"

namespace mlir {

/* Either an Attribute or a Value in the real class. */
class OpFoldResult {
public:
  int reserved;

  OpFoldResult();
  OpFoldResult(const Value &value);
  OpFoldResult(const Attribute &attribute);
};

} // namespace mlir
#endif
''',
    'mlir/IR/Operation.h': r'''#ifndef FC_MLIR_IR_OPERATION_H
#define FC_MLIR_IR_OPERATION_H

#include "mlir/IR/Block.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/Visitors.h"

namespace mlir {

class Operation {
public:
  Operation() : kind(0) {}

  int getKind() const { return kind; }
  MLIRContext *getContext() const;
  Location getLoc() const;
  ResultRange getResults() const;
  use_range getUses();
  bool use_empty() const;

  Value getOperand(int64_t index);
  OperandRange getOperands();
  int64_t getNumOperands() const;

  OpResult getResult(int64_t index);
  int64_t getNumResults() const;
  MutableOperandRange getOpOperands();
  void setOperand(int64_t index, Value value);

  /* `op->walk<WalkOrder::PreOrder>(lambda)` needs the generic form. */
  template <WalkOrder Order, class F>
  void walk(F callback) {}
  template <class F>
  void walk(F callback) {}

  void erase();
  void emitOpError(StringRef message);

protected:
  int kind;
};

template <class T>
bool isa(Operation *operation) {
  return T::classofOp(operation);
}

template <class T>
T dyn_cast(Operation *operation) {
  if (T::classofOp(operation))
    return T::castFrom(operation);
  return T();
}

template <class T>
T dyn_cast_or_null(Operation *operation) {
  if (operation == 0)
    return T();
  if (T::classofOp(operation))
    return T::castFrom(operation);
  return T();
}

} // namespace mlir
#endif
''',
    'mlir/IR/OperationSupport.h': r'''#ifndef FC_MLIR_IR_OPERATIONSUPPORT_H
#define FC_MLIR_IR_OPERATIONSUPPORT_H

#include "mlir/IR/Value.h"

namespace mlir {

class Operation;

class OpOperand {
public:
  int reserved;

  OpOperand();

  Operation *getOwner() const;
  bool is(Value value) const;
  void set(Value value);
};

} // namespace mlir
#endif
''',
    'mlir/IR/PatternMatch.h': r'''#ifndef FC_MLIR_IR_PATTERNMATCH_H
#define FC_MLIR_IR_PATTERNMATCH_H

#include "mlir/IR/Builders.h"
#include "mlir/Support/LogicalResult.h"

namespace mlir {

class PatternRewriter : public OpBuilder {
public:
  PatternRewriter();

  void eraseOp(Operation *op);
};

class ConversionPatternRewriter : public PatternRewriter {
public:
  ConversionPatternRewriter();

  void eraseOp(Operation *op);
};

/* Dialect-conversion pattern: matches on the root operation name rather than
 * on a C++ op type. */
class ConversionPattern {
public:
  int reserved;

  ConversionPattern(StringRef rootName, int benefit, MLIRContext *context);
  virtual ~ConversionPattern();

  virtual LogicalResult matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                                        ConversionPatternRewriter &rewriter) const;
};

template <class OpT>
class OpRewritePattern {
public:
  int reserved;

  explicit OpRewritePattern(MLIRContext *context) {}
  virtual ~OpRewritePattern() {}

  virtual LogicalResult matchAndRewrite(OpT op,
                                        PatternRewriter &rewriter) const;
};

class RewritePatternSet {
public:
  int reserved;

  explicit RewritePatternSet(MLIRContext *context);

  template <class T>
  void add(MLIRContext *context) {}

  template <class T>
  void add(MLIRContext *context, int64_t argument) {}
};

} // namespace mlir
#endif
''',
    'mlir/IR/Region.h': r'''#ifndef FC_MLIR_IR_REGION_H
#define FC_MLIR_IR_REGION_H

#include "mlir/IR/Block.h"

namespace mlir {

class Region {
public:
  int reserved;

  Region();

  Block &front();
  Block &back();
  bool empty() const;
  int64_t getNumArguments() const;
};

} // namespace mlir
#endif
''',
    'mlir/IR/TypeRange.h': r'''#ifndef FC_MLIR_IR_TYPERANGE_H
#define FC_MLIR_IR_TYPERANGE_H

#include <initializer_list>

#include "mlir/IR/Types.h"

namespace mlir {

/* Range of types: the op builders take one for the result types. */
class TypeRange {
public:
  int reserved;

  TypeRange();
  TypeRange(std::initializer_list<Type> types);

  Type operator[](fc_size_t index) const;
  fc_size_t size() const;
};

} // namespace mlir
#endif
''',
    'mlir/IR/Types.h': r'''#ifndef FC_MLIR_IR_TYPES_H
#define FC_MLIR_IR_TYPES_H

#include "mlir/IR/MLIRContext.h"

namespace mlir {

enum TypeKind {
  TypeNone = 0,
  TypeMemRef = 1,
  TypeInteger = 2,
  TypeFloat = 3,
  TypeVector = 4
};

class Type {
public:
  Type() : kind(TypeNone) {}
  explicit Type(int k) : kind(k) {}

  bool operator!() const { return kind == TypeNone; }
  explicit operator bool() const { return kind != TypeNone; }
  bool operator!=(const Type &other) const;
  int getKind() const { return kind; }

private:
  int kind;
};

template <class T>
bool isa(Type type) {
  return T::classof(type);
}

template <class T>
T dyn_cast(Type type) {
  if (T::classof(type))
    return T::castFrom(type);
  return T();
}

template <class T>
T cast(Type type) {
  return T::castFrom(type);
}

} // namespace mlir
#endif
''',
    'mlir/IR/TypeUtilities.h': r'''#ifndef FC_MLIR_IR_TYPEUTILITIES_H
#define FC_MLIR_IR_TYPEUTILITIES_H

#include "mlir/IR/BuiltinTypes.h"

namespace mlir {

/* Element type of a shaped type, or the type itself otherwise. */
Type getElementTypeOrSelf(Type type);
Type getElementTypeOrSelf(Value value);

} // namespace mlir
#endif
''',
    'mlir/IR/UseDefLists.h': r'''#ifndef FC_MLIR_IR_USEDEFLISTS_H
#define FC_MLIR_IR_USEDEFLISTS_H

#include "llvm/Support/Compiler.h"

namespace mlir {

class OpOperand;
class OpResult;
class Operation;

/* Non-owning view over a value's / operation's use-list (official use_range). */
class use_range {
public:
  int reserved;

  OpOperand *begin();
  OpOperand *end();
};

/* Non-owning view over an operation's results. */
class ResultRange {
public:
  int reserved;

  OpResult *begin();
  OpResult *end();
  fc_size_t size();
  OpResult &operator[](fc_size_t index);
};

/* Non-owning view over an operation's operands (read-only). */
class OperandRange {
public:
  int reserved;

  OpOperand *begin();
  OpOperand *end();
  fc_size_t size();
  OpOperand &operator[](fc_size_t index);
};

/* Non-owning view over a value's users. */
class user_range {
public:
  int reserved;

  Operation **begin();
  Operation **end();
};

/* Non-owning view over an operation's operands. */
class MutableOperandRange {
public:
  int reserved;

  OpOperand *begin();
  OpOperand *end();
  fc_size_t size();
  OpOperand &operator[](fc_size_t index);
};

} // namespace mlir
#endif
''',
    'mlir/IR/Value.h': r'''#ifndef FC_MLIR_IR_VALUE_H
#define FC_MLIR_IR_VALUE_H

#include "mlir/IR/Location.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/UseDefLists.h"

namespace mlir {

class Operation;
class Block;
class OpOperand;

enum ValueKind { ValueNone = 0, ValueBlockArg = 1, ValueOpResult = 2 };

class Value {
public:
  Value();

  bool operator!() const { return kind == ValueNone; }
  explicit operator bool() const { return kind != ValueNone; }

  int getKind() const { return kind; }
  Type getType() const;
  Location getLoc() const;

  Operation *getDefiningOp() const;
  Block *getParentBlock() const;
  use_range getUses() const;

  user_range getUsers() const;
  void replaceAllUsesWith(Value newValue);
  void replaceAllUsesExcept(Value newValue, Operation *exceptedUser);

  template <class T>
  T getDefiningOp() const {
    return T();
  }

private:
  int kind;
};

class BlockArgument : public Value {
public:
  BlockArgument();
  BlockArgument(const Value &other);

  Block *getOwner() const;

  static bool classof(Value value);
  static BlockArgument castFrom(Value value);
};

class OpResult : public Value {
public:
  OpResult();
  OpResult(const Value &other);

  Operation *getOwner() const;

  static bool classof(Value value);
  static OpResult castFrom(Value value);
};

template <class T>
bool isa(Value value) {
  return T::classof(value);
}

template <class T>
T dyn_cast(Value value) {
  if (T::classof(value))
    return T::castFrom(value);
  return T();
}

} // namespace mlir
#endif
''',
    'mlir/IR/ValueRange.h': r'''#ifndef FC_MLIR_IR_VALUERANGE_H
#define FC_MLIR_IR_VALUERANGE_H

#include <initializer_list>

#include "mlir/IR/Value.h"

namespace mlir {

/* Operand pack: the model accepts the brace-enclosed operand lists the passes
 * pass to the op builders. */
class ValueRange {
public:
  int reserved;

  ValueRange();
  ValueRange(const Value &value);
  ValueRange(std::initializer_list<Value> values);

  /* Explicit operand lists, so call sites can avoid the brace-initialised
   * initializer_list temporaries Frama-Clang mis-scopes. */
  ValueRange(const Value &v0, const Value &v1);
  ValueRange(const Value &v0, const Value &v1, const Value &v2);
  ValueRange(const Value &v0, const Value &v1, const Value &v2,
             const Value &v3);

  Value operator[](fc_size_t index) const;
  fc_size_t size() const;
};

} // namespace mlir
#endif
''',
    'mlir/IR/Visitors.h': r'''#ifndef FC_MLIR_IR_VISITORS_H
#define FC_MLIR_IR_VISITORS_H

#include "llvm/Support/Compiler.h"

namespace mlir {

enum class WalkOrder { PreOrder, PostOrder };

/* Result of a walk callback: `advance` keeps going, `interrupt` stops, and the
 * value converts to bool so `if (walkResult)` still works. */
class WalkResult {
public:
  int reserved;

  WalkResult();

  static WalkResult advance();
  static WalkResult interrupt();
  static WalkResult skip();

  explicit operator bool() const;
};

} // namespace mlir
#endif
''',
    'mlir/Pass/Pass.h': r'''#ifndef FC_MLIR_PASS_PASS_H
#define FC_MLIR_PASS_PASS_H

#include "llvm/Support/CommandLine.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"

namespace mlir {

class PassBase {
public:
  int reserved;

  PassBase();
  virtual ~PassBase();

  virtual StringRef getArgument() const;
  virtual StringRef getDescription() const;
  virtual void runOnOperation();
  virtual void getDependentDialects(DialectRegistry &registry) const;

  MLIRContext &getContext();
  void signalPassFailure();
  void markAllAnalysesPreserved();
};

template <class T>
using OperationPass = T;

template <class Derived, class T>
class PassWrapper : public PassBase {
public:
  int reserved;

  T getOperation();
};

template <class T>
class Option {
public:
  int reserved;

  Option(PassBase &parent, const char *name, llvm::cl::desc description,
         llvm::cl::init initial);

  bool operator!() const;

  /* Implicit conversion to the option value, e.g. Option<int64_t> -> int64_t
   * when the value is forwarded to a pattern constructor, or Option<bool> in
   * `if (option)`.  A separate explicit operator bool() would make the
   * signature collapse onto the same one when T = bool. */
  operator T() const;
  Option &operator=(const T &value);
};

template <class T>
class PassRegistration {
public:
  int reserved;

  PassRegistration();
};

} // namespace mlir

#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(x)
#endif
''',
    'mlir/Support/LLVM.h': r'''#ifndef FC_MLIR_SUPPORT_LLVM_H
#define FC_MLIR_SUPPORT_LLVM_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"

/* std::move stand-in: the model's sinks take lvalue references. */
namespace std {
template <class T>
T &move(T &value) {
  return value;
}

/* std::nullopt stand-in.  Frama-Clang 0.0.20 ships no <optional> header, so
 * the "no optional value" marker used for the ForallOp mapping is modelled
 * here instead of being pulled from the standard library. */
class nullopt_t {
public:
  int reserved;

  nullopt_t(int tag);
};
extern const nullopt_t nullopt;
} // namespace std

namespace mlir {

/* Frama-Clang 0.0.20 cannot parse `using llvm::X;` type imports, so the
 * re-exports are written as a typedef / alias templates instead. */
typedef llvm::StringRef StringRef;

template <class T, unsigned N = 0>
using SmallVector = llvm::SmallVector<T, N>;

template <class T>
using ArrayRef = llvm::ArrayRef<T>;

} // namespace mlir
#endif
''',
    'mlir/Support/LogicalResult.h': r'''#ifndef FC_MLIR_SUPPORT_LOGICALRESULT_H
#define FC_MLIR_SUPPORT_LOGICALRESULT_H

#include "mlir/Support/LLVM.h"

namespace mlir {

class LogicalResult {
public:
  int reserved;

  LogicalResult();
};

LogicalResult failure();
LogicalResult success();
bool failed(LogicalResult result);

} // namespace mlir
#endif
''',
    'mlir/Transforms/DialectConversion.h': r'''#ifndef FC_MLIR_TRANSFORMS_DIALECTCONVERSION_H
#define FC_MLIR_TRANSFORMS_DIALECTCONVERSION_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Support/LogicalResult.h"

namespace mlir {

/* Legal-target description used by the apply*Conversion family. */
class ConversionTarget {
public:
  int reserved;

  explicit ConversionTarget(MLIRContext &context);

  template <class... Dialects>
  void addLegalDialect() {}

  template <class... Ops>
  void addLegalOp() {}
};

/* Generic stand-in for the apply*Conversion family: the concrete target type
 * is deduced from the call site. */
template <class OpT, class TargetT>
LogicalResult applyPartialConversion(OpT op, TargetT &target,
                                     RewritePatternSet &patterns);

} // namespace mlir
#endif
''',
    'mlir/Transforms/GreedyPatternRewriteDriver.h': r'''#ifndef FC_MLIR_TRANSFORMS_GREEDY_PATTERN_REWRITE_DRIVER_H
#define FC_MLIR_TRANSFORMS_GREEDY_PATTERN_REWRITE_DRIVER_H

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/PatternMatch.h"

namespace mlir {

enum GreedyRewriteStrictness { AnyOp, ExistingAndNewOps };

class GreedyRewriteConfig {
public:
  int reserved;

  GreedyRewriteConfig();

  void setStrictness(GreedyRewriteStrictness strictness);
};

LogicalResult applyPatternsGreedily(func::FuncOp func,
                                    RewritePatternSet &patterns,
                                    GreedyRewriteConfig config);

} // namespace mlir
#endif
''',
    'RVV/RVVDialect.h': r'''#ifndef FC_RVV_RVVDIALECT_H
#define FC_RVV_RVVDIALECT_H

#include "mlir/IR/Operation.h"

namespace mlir {
namespace rvv {

class RVVDialect {
public:
  int reserved;
};

} // namespace rvv
} // namespace mlir
#endif
''',
    'RVV/Transforms.h': r'''#ifndef FC_RVV_TRANSFORMS_H
#define FC_RVV_TRANSFORMS_H

#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/IR/PatternMatch.h"

namespace mlir {
namespace buddy {

void configureRVVLegalizeForExportTarget(LLVMConversionTarget &target);

void populateRVVLegalizeForLLVMExportPatterns(LLVMTypeConverter &converter,
                                              RewritePatternSet &patterns,
                                              int64_t indexBitwidth);

} // namespace buddy
} // namespace mlir
#endif
''',
    'vector': r'''#ifndef FC_STD_VECTOR
#define FC_STD_VECTOR

#include <initializer_list>

#include "llvm/Support/Compiler.h"

/* Stand-in for the C++ standard <vector> header: the front-end ships no such
 * header, so the subset the passes use is modelled here. */
namespace std {

template <class T>
class vector {
public:
  typedef T *iterator;
  typedef const T *const_iterator;

  int reserved;

  vector();
  vector(const vector &other);
  vector &operator=(const vector &other);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  bool empty() const;
  fc_size_t size() const;

  T &operator[](fc_size_t index);
  const T &operator[](fc_size_t index) const;

  void push_back(const T &value);
};

} // namespace std
#endif
''',
}
# === MODEL END ===


if __name__ == '__main__':
    main()
