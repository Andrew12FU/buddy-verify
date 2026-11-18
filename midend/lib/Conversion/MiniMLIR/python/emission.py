"""Apply an AST slicing plan to shared header bytes; never synthesize C++ APIs."""

import hashlib
import json
from pathlib import Path
import re


def digest(path: Path) -> str:
    """Return the SHA-256 of an existing source/header; I/O failures propagate."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def include_closure(root: Path, source: Path) -> list[Path]:
    """Resolve the model's literal include graph, rejecting external fallbacks.

    Args:
        root: Canonical include tree. Only literal, library-owned includes are
            supported in this bounded model; macro include names are rejected.
        source: Pass whose original include spellings are the entry headers.
    Returns:
        Sorted library-relative paths, including transitive prerequisites.
    """
    pending = [(source, source.read_text())]
    seen: set[Path] = set()
    while pending:
        owner, text = pending.pop()
        for line in text.splitlines():
            if not re.match(r"\s*#\s*include\b", line):
                continue
            match = re.fullmatch(r'\s*#\s*include\s*[<"]([^>"]+)[>"]\s*(?://.*)?', line)
            if not match:
                raise ValueError(f"Unsupported include directive in {owner}: {line}")
            relative = Path(match[1])
            path = (root / relative).resolve()
            if not path.is_relative_to(root) or not path.is_file():
                raise ValueError(f"MiniMLIR does not own dependency {relative} required by {owner}")
            if relative not in seen:
                seen.add(relative)
                pending.append((path, path.read_text()))
    return sorted(seen)


def materialize(root: Path, source: Path, plan_path: Path, output: Path) -> dict:
    """Emit a new pass include tree by deleting AST-selected declarations/members.

    Args:
        root: Sole canonical definition source; never modified.
        source: Rewritten pass, with original include paths retained.
        plan_path: JSON from buddy-mini-slice, parsed against exactly this root.
        output: New/nonexistent directory; existing results cannot be overwritten.
    Returns:
        Manifest containing source hashes, retained reasons, removals and output
        hashes. Kept text is byte-for-byte canonical text, including constraints,
        defaults, bases, fields and inline bodies. Nested removals merge by range.
    Raises:
        ValueError: Plan/path/range inconsistency; missing owned dependency.
        OSError: Read/create/write failure.
    """
    root, output = root.resolve(), output.resolve()
    if output.exists() or root.is_relative_to(output) or output.is_relative_to(root):
        raise ValueError("Extraction requires a new directory outside the canonical include tree")
    plan = json.loads(plan_path.read_text())
    if plan.get("parsed") is not True or Path(plan["library_root"]).resolve() != root:
        raise ValueError("Slicing plan does not describe the selected canonical library")
    expected_headers = plan.get("canonical_headers_sha256")
    current_headers = {str(p): digest(p) for p in sorted(root.rglob("*")) if p.is_file()}
    if expected_headers != current_headers or plan.get("input_sha256") != digest(source):
        raise ValueError("Source or canonical headers changed since AST extraction; regenerate the plan")
    files = include_closure(root, source)
    # Model abstraction is maintained once, before AST slicing. Keep its audit
    # distinct from generated conversion rewrites, with a per-run snapshot.
    registry = root.parent / "docs/interface-abstractions.json"
    abstractions = None
    if registry.is_file():
        abstractions = json.loads(registry.read_text())
        for edit in abstractions["edits"]:
            header = (root / edit["file"]).resolve()
            if not header.is_relative_to(root) or digest(header) != edit["after_sha256"]:
                raise ValueError("Canonical abstraction record is stale; update the audited model record")
        for extension in abstractions.get("extensions", []):
            header = (root / extension["file"]).resolve()
            if not header.is_relative_to(root) or digest(header) != extension["after_sha256"]:
                raise ValueError("Canonical interface extension record is stale")
        abstractions["registry_source"] = {"path": str(registry), "sha256": digest(registry)}
        abstractions["headers_in_include_closure"] = sorted(
            {edit["file"] for edit in abstractions["edits"]} & {str(p) for p in files})
    by_file: dict[Path, list[dict]] = {}
    for item in plan["declarations"]:
        path = Path(item["file"]).resolve()
        if not path.is_relative_to(root):
            raise ValueError(f"Slicing plan attempts to edit an external header: {path}")
        by_file.setdefault(path.relative_to(root), []).append(item)
    prepared: dict[Path, bytes] = {}
    removed, retained = [], []
    for relative in files:
        original = (root / relative).read_bytes()
        spans = []
        for item in by_file.get(relative, []):
            begin, end = item["begin"], item["end"]
            if not 0 <= begin < end <= len(original):
                raise ValueError(f"Invalid declaration range in {relative}: {begin}:{end}")
            record = {**item, "file": str(relative)}
            (retained if item["keep"] else removed).append(record)
            if not item["keep"]:
                spans.append((begin, end))
        merged: list[tuple[int, int]] = []
        for begin, end in sorted(set(spans)):
            if merged and begin <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(end, merged[-1][1]))
            else:
                merged.append((begin, end))
        # A parent deletion may subsume a child's structural keep. It may not
        # subsume a referenced member: that would indicate an ownership bug.
        for item in by_file.get(relative, []):
            if item["keep"] and item["reason"] not in {"lookup infrastructure"}:
                if any(a <= item["begin"] and item["end"] <= b for a, b in merged):
                    raise ValueError(f"Retained declaration removed with its owner: {item['name']}")
                if item["kind"] in {"Function", "FunctionTemplate", "CXXMethod",
                                    "CXXConstructor", "CXXDestructor", "CXXConversion"}:
                    if any(item["begin"] < a < item["end"] for a, _ in merged):
                        raise ValueError(f"Attempt to crop a retained function body: {item['name']}")
        sliced = original
        for begin, end in reversed(merged):
            # Preserve line numbers and separate neighboring tokens after removal.
            sliced = sliced[:begin] + b" " + b"\n" * original[begin:end].count(b"\n") + sliced[end:]
        prepared[relative] = sliced
    output.mkdir(parents=True)
    for relative, content in prepared.items():
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
    abstraction_reference = None
    if abstractions is not None:
        snapshot = output.parent / "interface-abstractions.json"
        snapshot.write_text(json.dumps(abstractions, ensure_ascii=False, indent=2) + "\n")
        abstraction_reference = {"path": snapshot.name, "sha256": digest(snapshot)}
    manifest = {
        "schema_version": 1, "library_root": str(root), "source": str(source),
        "source_sha256": digest(source), "plan_sha256": digest(plan_path),
        "granularity": "declarations and ordinary members; structural/overload closure retained",
        "canonical_headers_sha256": {str(p): digest(root / p) for p in files},
        "generated_headers_sha256": {str(p): digest(output / p) for p in files},
        "retained": retained, "removed": removed,
        "canonical_abstractions": abstraction_reference,
    }
    (output.parent / "interface-manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    return manifest
