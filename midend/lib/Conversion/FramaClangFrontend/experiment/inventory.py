"""把 pass 源码改写与头文件改写分开记账，保留供下一阶段使用的来源链。"""

from collections import Counter
import json
from pathlib import Path

from .artifacts import digest, save_json


def write_inventory(out: Path, profile: str, state: str) -> dict | None:
    """Save a per-run, machine-readable ledger without editing source files.

    Args:
        out: Current run directory; historical-directory protection is owned by
            the runner. Partial/failed runs are recorded when reports exist.
        profile: Audited pass identifier.
        state: Current run status; rewriting success is separate from conversion.
    Returns:
        Summary reference with counts/hash, or None before a source report exists.
        Full edits, constraints/provenance and coordinate conventions remain in
        rewrite-inventory.json, with hashes of the original detailed reports.
    """
    source_path = out / "rewrite.json"
    if not source_path.is_file():
        return None
    source = json.loads(source_path.read_text())
    applied = source.get("status") == "rewritten" and (out / "rewritten.cpp").is_file()
    source_edits = source.get("edits", []) if applied else []
    header_path = out / "header-rewrite.json"
    headers = json.loads(header_path.read_text()) if header_path.is_file() else None
    header_edits = headers["edits"] if headers else []

    def reference(name: str) -> dict | None:
        path = out / name
        return {"path": name, "sha256": digest(path)} if path.is_file() else None

    source_counts = dict(Counter(edit["rule"] for edit in source_edits))
    header_counts = dict(Counter(edit["rule"] for edit in header_edits))
    inventory = {
        "schema_version": 1, "pass": profile, "run_state": state,
        "source_rewrites": {
            "applied": applied, "input": source["input"],
            "original_snapshot": reference("original.cpp"),
            "rewritten_source": reference("rewritten.cpp"),
            "report": reference("rewrite.json"),
            "offset_basis": source.get("offset_basis", "UTF-8 bytes against the original source, before all edits"),
            "rounds": source.get("rounds", []),
            "edit_count": len(source_edits), "rule_counts": source_counts,
            "edits": source_edits,
        },
        "header_rewrites": {
            "applied": headers is not None, "report": reference("header-rewrite.json"),
            "patch": reference("header-rewrite.patch"),
            "input_root": "include", "conversion_root": "conversion-include",
            "offset_basis": headers["offset_basis"] if headers else None,
            "edit_count": len(header_edits), "rule_counts": header_counts,
            "changed_headers": sorted({edit["file"] for edit in header_edits}),
            "edits": header_edits,
        },
        "extraction_inputs": {
            "rewritten_pass": reference("rewritten.cpp"),
            "declaration_dependency_plan": reference("interface-plan.json"),
            "canonical_to_extracted_manifest": reference("interface-manifest.json"),
            "canonical_interface_abstractions": reference("interface-abstractions.json"),
            "extracted_to_conversion_rewrites": reference("header-rewrite.json"),
            "boundary": "Canonical interface abstraction and converter compatibility are separate stages; this ledger is not an arbitrary real-MLIR extractor",
        },
    }
    path = out / "rewrite-inventory.json"
    save_json(path, inventory)
    return {"path": path.name, "sha256": digest(path),
            "source_edit_count": len(source_edits), "source_rule_counts": source_counts,
            "header_edit_count": len(header_edits), "header_rule_counts": header_counts}
