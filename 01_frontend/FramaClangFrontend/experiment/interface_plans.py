"""Combine original/rewritten dependency roots without synthesizing header APIs."""

import json
from pathlib import Path

from .artifacts import digest, save_json


def merge_dependency_roots(primary: Path, baseline: Path) -> None:
    """Retain declarations required by either version of one pass.

    Args:
        primary: Plan for rewritten.cpp; updated in place before materialization.
        baseline: Temporary plan for the unmodified original source, read only.
    Returns:
        None. The merged plan keeps the rewritten source identity and records
        the original source hash. No declaration text is added or modified.
    Raises:
        ValueError: Plans were parsed against different canonical library bytes.
    """
    result = json.loads(primary.read_text())
    original = json.loads(baseline.read_text())
    for field in ("library_root", "canonical_headers_sha256"):
        if result[field] != original[field]:
            raise ValueError("Dependency plans use different canonical headers")
    if not result["parsed"] or not original["parsed"]:
        raise ValueError("Cannot merge an unparsed dependency plan")
    declarations = {}
    for plan in (result, original):
        for item in plan["declarations"]:
            key = (item["file"], item["begin"], item["end"], item["kind"], item["name"])
            old = declarations.get(key)
            if old is None or item["keep"]:
                declarations[key] = item
    result["declarations"] = list(declarations.values())
    result["additional_dependency_input"] = {
        "input": original["input"], "sha256": original["input_sha256"],
        "plan_sha256": digest(baseline),
        "policy": "union of original and rewritten declaration dependencies",
    }
    save_json(primary, result)
