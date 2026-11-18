#!/usr/bin/env python3
"""为下载后的 Reshape 源码重定位一条真实编译记录，保留外部依赖路径。"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex


def main() -> None:
    """解析路径参数；写出单条编译数据库。版本或输入不匹配时退出。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True, help='下载后的 buddy-verify 根目录')
    parser.add_argument('--database', type=Path, required=True, help='完整 Buddy 构建生成的真实编译数据库')
    parser.add_argument('--output', type=Path, required=True, help='只用于本次适配的编译数据库文件')
    args = parser.parse_args()
    relative = Path('midend/lib/Conversion/GraphRedundancyElimination/SimplifyTosaReshape.cpp')
    source = (args.repo / relative).resolve()
    entries = json.loads(args.database.read_text())
    matches = [e for e in entries if Path(e['file']).as_posix().endswith('/' + relative.as_posix())]
    if len(matches) != 1:
        raise SystemExit('需要且只能找到一条 Reshape 编译记录')
    entry = dict(matches[0])
    original = Path(entry['file']).resolve()
    # 防止将另一版本 pass 的编译环境悄悄用于本次输入。
    if original.read_bytes() != source.read_bytes():
        raise SystemExit('下载源码与外部 Buddy 的 pass 内容不同，需先审查版本匹配')
    argv = entry.get('arguments') or shlex.split(entry['command'])
    changed = [i for i, token in enumerate(argv) if token == str(original)]
    if len(changed) != 1:
        raise SystemExit('真实编译记录没有唯一的绝对源码参数，拒绝猜测替换')
    argv[changed[0]] = str(source)
    entry.pop('command', None)
    entry['file'] = str(source)
    entry['arguments'] = argv
    # directory 和 -I 路径仍指向完整 Buddy/LLVM 源码与生成头，不能整体替换。
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps([entry], ensure_ascii=False, indent=2) + '\n')
    print(f'编译记录：{args.output.resolve()}')
    print(f'本轮输入：{source}')
    print(f'源码 SHA-256：{hashlib.sha256(source.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
