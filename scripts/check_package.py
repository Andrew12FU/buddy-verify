#!/usr/bin/env python3
"""Check packaged source/data against the review manifest; ignores local build outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
a = p.parse_args(); root = a.root.resolve()
manifest = json.loads((root / "package-manifest.json").read_text())
errors = []
for name, expected in manifest.items():
    rel = Path(name)
    if rel.is_absolute() or ".." in rel.parts:
        errors.append(name + ": unsafe path"); continue
    file = root / rel
    if not file.is_file(): errors.append(name + ": missing"); continue
    if hashlib.sha256(file.read_bytes()).hexdigest() != expected:
        errors.append(name + ": checksum differs")
if errors:
    print("\n".join(errors), file=sys.stderr); sys.exit(1)
print(f"Verified {len(manifest)} packaged files")
