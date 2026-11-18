#!/usr/bin/env python3
"""从已提取的专用库生成 Frama-Clang 转换用头文件；共享原库只读。"""

import argparse
from pathlib import Path

from python.rewrites.pipeline import rewrite_headers


def main() -> None:
    """Read explicit paths and emit a fresh conversion include tree and report."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = rewrite_headers(args.source, args.manifest, args.output)
    print(f"Rewrote {len(report['edits'])} declarations in {len(report['output_headers_sha256'])} headers")


if __name__ == "__main__":
    main()
