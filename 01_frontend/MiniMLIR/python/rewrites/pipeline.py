"""验证专用库来源，按顺序改写内存副本，输出独立的转换用 include 树。"""

import difflib
import json
from pathlib import Path

from ..emission import digest
from .constructors import expand
from .typed_attr import specialize
from .bounded_interfaces import specialize_bounded
from .nested_records import inline_use_iterator
from .walk import expand_walk_returns
from .indexed_ranges import specialize_indexed_ranges
from .conversion_pattern import expand_conversion_pattern


def rewrite_headers(source: Path, manifest_path: Path, output: Path) -> dict:
    """Create a new conversion library and its complete rewrite record.

    Args:
        source: Extracted include tree, never modified.
        manifest_path: Its extraction manifest, used to reject stale/missing files.
        output: New directory outside source and the canonical library.
    Returns:
        Report containing edits, preserved limitations and input/output hashes.
        The sibling header-rewrite.json and header-rewrite.patch are written too.
    Raises:
        ValueError: Provenance mismatch, unsafe output or an unaudited rule shape.
        OSError: File access failure; no original files are overwritten.
    """
    source, output = source.resolve(), output.resolve()
    manifest = json.loads(manifest_path.read_text())
    canonical = Path(manifest["library_root"]).resolve()
    if (output.exists() or source.is_relative_to(output) or output.is_relative_to(source)
            or canonical.is_relative_to(output) or output.is_relative_to(canonical)):
        raise ValueError("Header rewrites require a new directory outside source and canonical trees")
    if any((output.parent / name).exists() for name in ["header-rewrite.json", "header-rewrite.patch"]):
        raise ValueError("Historical header rewrite reports cannot be overwritten")
    paths = sorted(p for p in source.rglob("*") if p.is_file())
    actual = {str(p.relative_to(source)): digest(p) for p in paths}
    if actual != manifest["generated_headers_sha256"]:
        raise ValueError("Extracted headers changed since materialization; regenerate them")
    if any(not p.resolve().is_relative_to(source) for p in paths):
        raise ValueError("Extracted header points outside its owned include tree")
    before = {str(p.relative_to(source)): p.read_text() for p in paths}
    after = dict(before)
    edits = specialize(after)
    edits += specialize_bounded(after)
    edits += expand(after)
    edits += expand_conversion_pattern(after)
    edits += specialize_indexed_ranges(after)
    edits += inline_use_iterator(after)
    edits += expand_walk_returns(after)
    for sequence, edit in enumerate(edits):
        edit["sequence"] = sequence
    patch = "".join("".join(difflib.unified_diff(
        before[name].splitlines(keepends=True), after[name].splitlines(keepends=True),
        fromfile="include/" + name, tofile="conversion-include/" + name))
        for name in before if before[name] != after[name])
    # Rules finish in memory before any output directory is created.
    output.mkdir(parents=True)
    for name, text in after.items():
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
    report = {
        "schema_version": 1, "source_root": str(source), "output_root": str(output),
        "extraction_manifest_sha256": digest(manifest_path),
        "input_headers_sha256": actual,
        "output_headers_sha256": {name: digest(output / name) for name in after},
        "edits": edits,
        "offset_basis": "UTF-8 bytes after preceding edits in sequence order",
        "constructor_policy": "Expand audited constructors in the conversion copy only; canonical inherited constructors remain unchanged",
        "limits": ["Bounded MiniMLIR declaration rewrites, not a general C++ template expander",
                   "Expanded constructor declarations do not model external construction algorithms",
                   "External function_ref dispatch/storage are abstracted in the canonical model; its local-lambda constructor body is an adapter, not a behavioral contract"],
    }
    (output.parent / "header-rewrite.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    (output.parent / "header-rewrite.patch").write_text(patch)
    return report
