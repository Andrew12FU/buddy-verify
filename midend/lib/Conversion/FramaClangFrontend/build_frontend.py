#!/usr/bin/env python3
"""Build the default Clang 18 frontend; preserve historical 18/23 build trees."""

import argparse
from pathlib import Path
import subprocess

from experiment.profiles import FRONTEND_LLVM, FRONTEND_ROOT, WORKSPACE
from experiment.toolchain import compiler_environment


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--llvm-prefix", type=Path, default=FRONTEND_LLVM)
    parser.add_argument("--llvm-major", choices=["18", "23"], default="18")
    parser.add_argument("--build-dir", type=Path, default=FRONTEND_ROOT / "build-llvm18")
    parser.add_argument("--jobs", type=int, default=1)
    args = parser.parse_args()
    llvm, build = args.llvm_prefix.resolve(), args.build_dir.resolve()
    if build == (FRONTEND_ROOT / "build").resolve():
        parser.error("Keep the historical build/ baseline; choose a separate build directory")
    cmake = WORKSPACE / "environment-v0.0.4/env/bin/cmake"
    ninja = WORKSPACE / "environment-v0.0.4/env/bin/ninja"
    env = compiler_environment(llvm)
    subprocess.run([str(cmake), "-S", str(FRONTEND_ROOT), "-B", str(build), "-G", "Ninja",
                    f"-DCMAKE_MAKE_PROGRAM={ninja}", "-DCMAKE_BUILD_TYPE=Release",
                    f"-DCMAKE_C_COMPILER={llvm / 'bin/clang'}",
                    f"-DCMAKE_CXX_COMPILER={llvm / 'bin/clang++'}",
                    f"-DFRONTEND_LLVM_MAJOR={args.llvm_major}",
                    f"-DLLVM_DIR={llvm / 'lib/cmake/llvm'}",
                    f"-DClang_DIR={llvm / 'lib/cmake/clang'}"], env=env, check=True)
    subprocess.run([str(cmake), "--build", str(build), "--parallel", str(args.jobs)],
                   env=env, check=True)


if __name__ == "__main__":
    main()
