#!/usr/bin/env python3
"""Rewrite a bundled pass or an explicit input, optionally convert and read back C."""
import argparse
import os
from dataclasses import replace
from pathlib import Path
import subprocess
import sys
from experiment.profiles import (CONVERSION_CIRGEN, CONVERSION_LLVM, FRONTEND_LLVM,
    FRONTEND_ROOT, FRONTEND_TOOL, PASS_PROFILES, SYSTEM_PREFIX, RunConfig)
from experiment.paths import executable_path
from experiment.validation import ExperimentRunner
from experiment.output import output_directory


def parse_config(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pass", dest="pass_name", choices=PASS_PROFILES, default="reshape")
    parser.add_argument("--input", type=Path, help="Custom input; disables bundled counts/reference checks")
    parser.add_argument("--rules", help="Explicit comma-separated rules; constructor is opt-in; disables bundled count/reference checks")
    parser.add_argument("--parse-mode", choices=["model", "real"], default="model",
                        help="model: bundled MiniMLIR; real: caller's compilation database")
    parser.add_argument("--llvm-prefix", type=Path, default=FRONTEND_LLVM)
    parser.add_argument("--frama-llvm-prefix", type=Path, default=Path(os.environ["FRAMA_LLVM_PREFIX"]) if os.environ.get("FRAMA_LLVM_PREFIX") else None)
    parser.add_argument("--system-prefix", type=Path, default=SYSTEM_PREFIX,
                        help="Optional audited Conda sysroot/GCC prefix; omitted uses compiler defaults")
    parser.add_argument("--tool", type=Path, default=FRONTEND_TOOL)
    parser.add_argument("--compile-commands", type=Path)
    parser.add_argument("--database-source", type=Path,
                        help="Original file key in the external database when input is a copied snapshot")
    parser.add_argument("--extra-arg", action="append", default=[], help="Repeat; use --extra-arg=-DFLAG")
    parser.add_argument("--extra-include", type=Path, action="append", default=[])
    parser.add_argument("--frama-c", type=Path, default=executable_path("FRAMA_C", "frama-c"))
    parser.add_argument("--frama-cirgen", type=Path, default=CONVERSION_CIRGEN)
    parser.add_argument("--output-dir", type=Path, required=True, help="New or empty output directory")
    parser.add_argument("--skip-frama", action="store_true", help="Native checks only; conversion remains unverified")
    args = parser.parse_args(argv)
    args.frama_llvm_prefix = args.frama_llvm_prefix or args.llvm_prefix
    profile = PASS_PROFILES[args.pass_name]
    if args.input or args.rules is not None:
        profile = replace(profile, source=args.input.resolve() if args.input else profile.source,
            rules=args.rules if args.rules is not None else profile.rules, expected=None,
            reference=None, reference_source=None, require_reference_match=False, rounds=(),
            namespace_prefix=args.input.stem if args.input else profile.namespace_prefix)
    if args.parse_mode == "real" and args.compile_commands is None:
        parser.error("--parse-mode real requires --compile-commands")
    if args.parse_mode == "model" and (args.compile_commands or args.database_source):
        parser.error("Compilation databases require --parse-mode real")
    llvm, tool, out = args.llvm_prefix.resolve(), args.tool.resolve(), args.output_dir.resolve()
    if out.exists() and (not out.is_dir() or any(out.iterdir())):
        parser.error("Output directory must be new or empty")
    if out == profile.source.resolve() or out in profile.source.resolve().parents:
        parser.error("Input must be outside the output directory")
    required = [tool, profile.source, tool.parent / "buddy-mini-slice", llvm / "bin/clang++"]
    if args.parse_mode == "real": required.append(args.compile_commands)
    if not args.skip_frama:
        required += [args.frama_llvm_prefix / "bin/clang++", args.frama_c, args.frama_cirgen]
    for path in required:
        if not path.is_file(): parser.error(f"Required file is missing: {path}")
    extra = tuple(args.extra_arg) + tuple("-I" + str(p.resolve()) for p in args.extra_include)
    return RunConfig(profile=profile, llvm=llvm, tool=tool, interface=None,
        database=args.compile_commands.resolve() if args.compile_commands else None,
        frama_c=args.frama_c.resolve(), out=out, skip_frama=args.skip_frama,
        frama_llvm=args.frama_llvm_prefix.resolve(),
        system_prefix=args.system_prefix.resolve() if args.system_prefix else None,
        frama_cirgen=args.frama_cirgen.resolve(), replace_output=False, parse_mode=args.parse_mode,
        compile_source=args.database_source.resolve() if args.database_source else None, extra_arguments=extra)


def main(argv=None):
    try:
        config = parse_config(argv)
        with output_directory(config.out, replace=False):
            return ExperimentRunner(config).run()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"Failed to initialize frontend: {error}", file=sys.stderr)
        return 1

if __name__ == "__main__": sys.exit(main())
