#!/usr/bin/env python3
"""Generate a pass include tree from a completed buddy-mini-slice AST plan."""
import argparse
from pathlib import Path
from python.emission import materialize


def main() -> None:
    """Parse explicit paths and emit a new include tree; failures stay visible."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library-root", type=Path, default=Path(__file__).resolve().parent / "include")
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = materialize(args.library_root, args.source, args.plan, args.output)
    print(f"Extracted {len(result['generated_headers_sha256'])} headers; "
          f"removed {len(result['removed'])} declaration/member ranges")


if __name__ == "__main__":
    main()
