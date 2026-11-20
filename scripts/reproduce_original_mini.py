#!/usr/bin/env python3
"""Convert an existing rewritten pass with its untouched historical mini_mlir.h."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "01_frontend/FramaClangFrontend"))
from experiment.paths import FRONTEND_LLVM, executable_path
from experiment.toolchain import compiler_environment, resource_directory, run_command


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pass", dest="name", choices=["reshape", "matmul-scalar", "lower-ime", "silu-fusion"], default="reshape")
    parser.add_argument("--source", type=Path, help="Existing frontend rewritten.cpp; defaults to 02_passes snapshot")
    parser.add_argument("--reshape-mini", choices=["v1", "v2"], default="v1",
                        help="Reshape historical header: v1=270 lines; v2=1818 lines")
    parser.add_argument("--llvm-prefix", type=Path, default=FRONTEND_LLVM)
    parser.add_argument("--frama-c", type=Path, default=executable_path("FRAMA_C", "frama-c"))
    parser.add_argument("--frama-cirgen", type=Path, default=executable_path("FRAMA_CIRGEN", "framaCIRGen"))
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    source = (args.source or ROOT / "02_passes" / args.name / "rewritten.cpp").resolve()
    header = ROOT / "03_manual_passes/buddy_four" / args.name / "mini_mlir.h"
    if args.name == "reshape" and args.reshape_mini == "v1":
        header = header.parent / "v1/mini_mlir.h"
    llvm, frama, converter, out = args.llvm_prefix.resolve(), args.frama_c.resolve(), args.frama_cirgen.resolve(), args.output_dir.resolve()
    for p in [source, header, llvm / "bin/clang++", frama, converter]:
        if not p.is_file(): parser.error(f"Required file is missing: {p}")
    if out.exists() and (not out.is_dir() or any(out.iterdir())):
        parser.error("Output directory must be new or empty")
    if any(out == p or out in p.parents for p in [source, header]):
        parser.error("Input files must be outside output directory")
    original_source, original_header = digest(source), digest(header)
    # Only redirect literal include lines; never alter statements or header bytes.
    chunks, redirects, offset = [], [], 0
    for line in source.read_bytes().splitlines(keepends=True):
        if re.match(rb"[ \t]*#[ \t]*include\b", line):
            if not re.fullmatch(rb'[ \t]*#[ \t]*include[ \t]*[<"][^>"\r\n]+[>"][ \t]*(?://[^\r\n]*)?\r?\n?', line):
                parser.error("Only literal include directives are supported")
            replacement = b'#include "mini_mlir.h"\n' if not redirects else b''
            chunks.append(replacement)
            redirects.append({"offset": offset, "before": line.decode(), "after": replacement.decode()})
        else:
            chunks.append(line)
        offset += len(line)
    if not redirects: parser.error("Input has no literal include directives")
    resource = resource_directory(llvm)
    out.mkdir(parents=True, exist_ok=True); (out / "logs").mkdir()
    (out / "input.cpp").write_bytes(b''.join(chunks))
    (out / "mini_mlir.h").write_bytes(header.read_bytes())
    flags = ["-std=c++17", "-I" + str(out), "-resource-dir=" + str(resource)]
    env = compiler_environment(llvm, frama)
    summary = {"pass": args.name, "workflow": "frontend output plus untouched original single-header model",
        "source": str(source), "source_sha256": original_source,
        "original_header": str(header), "original_header_sha256": original_header,
        "copied_header_sha256": digest(out / "mini_mlir.h"), "source_rules_rerun": False,
        "header_rewrites_applied": False, "include_redirects": redirects,
        "reshape_mini_version": args.reshape_mini if args.name == "reshape" else None,
        "status": "in_progress", "stages": {}}
    def save():
        (out / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n")
    def stage(name, cmd):
        result = run_command(cmd, out / "logs" / (name + ".log"), cwd=out, env=env)
        summary["stages"][name] = result; save()
        if result["exit_code"]: raise RuntimeError(f"{name} failed; inspect {result['log']}")
    try:
        save()
        stage("native_syntax", [llvm / "bin/clang++", *flags, "-fsyntax-only", out / "input.cpp"])
        stage("frama_conversion", [frama, out / "input.cpp", "-cxx-nostdinc", "-machdep", "gcc_x86_64",
            "-cxx-clang-command=" + str(converter), "-fclang-cpp-extra-args=" + shlex.join(flags),
            "-cxx-parseable-output", "-print", "-ocode", out / "converted.c"])
        c = out / "converted.c"
        if not c.is_file() or not c.stat().st_size: raise RuntimeError("Conversion produced no nonempty C")
        stage("frama_c_readback", [frama, c, "-machdep", "gcc_x86_64"])
        if digest(source) != original_source or digest(header) != original_header:
            raise RuntimeError("Original input changed during validation")
        summary.update(status="converted", c_lines=len(c.read_bytes().splitlines()), c_bytes=c.stat().st_size,
                       source_and_original_header_unchanged=True)
        save(); print(json.dumps({"status": "converted", "c_lines": summary["c_lines"], "output": str(out)}, ensure_ascii=False))
        return 0
    except (OSError, ValueError, RuntimeError) as error:
        summary.update(status="failed", error=str(error)); save()
        print(str(error), file=sys.stderr); return 1


if __name__ == "__main__":
    try: sys.exit(main())
    except (OSError, ValueError) as error:
        print(str(error), file=sys.stderr); sys.exit(1)
