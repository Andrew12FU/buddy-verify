#!/usr/bin/env bash
# 构建下载后的前端，再运行完整 Reshape 流程。环境变量可覆盖本机路径。
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(dirname -- "$SCRIPT_DIR")"
WORKSPACE_ROOT="${WORKSPACE_ROOT:-/home/ce/Desktop/BuddyCompiler}"
LLVM18="${LLVM18:-/home/ce/Desktop/framaclang/environment/llvm18}"
FRAMA_PREFIX="${FRAMA_PREFIX:-/home/ce/.opam/framac-study}"
SYS_PREFIX="${SYS_PREFIX:-$LLVM18}"
COMPILE_DB="${COMPILE_DB:-$WORKSPACE_ROOT/build-v0.0.4/buddy/compile_commands.json}"
CMAKE="${CMAKE:-$WORKSPACE_ROOT/environment-v0.0.4/env/bin/cmake}"
NINJA="${NINJA:-$WORKSPACE_ROOT/environment-v0.0.4/env/bin/ninja}"
FRONTEND="$REPO/midend/lib/Conversion/FramaClangFrontend"
CASE_DIR="$SCRIPT_DIR/reshape"
export PATH="$LLVM18/bin:$FRAMA_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$LLVM18/lib:$FRAMA_PREFIX/lib/stublibs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$CMAKE" -S "$FRONTEND" -B "$FRONTEND/build-llvm18" -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$NINJA" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$LLVM18/bin/clang" -DCMAKE_CXX_COMPILER="$LLVM18/bin/clang++" \
  -DFRONTEND_LLVM_MAJOR=18 -DLLVM_DIR="$LLVM18/lib/cmake/llvm" \
  -DClang_DIR="$LLVM18/lib/cmake/clang"
"$CMAKE" --build "$FRONTEND/build-llvm18" \
  --target buddy-framac-rewrite buddy-mini-slice --parallel 1
python3 "$SCRIPT_DIR/prepare_compile_commands.py" \
  --repo "$REPO" --database "$COMPILE_DB" --output "$CASE_DIR/compile_commands.json"
python3 "$FRONTEND/run.py" --pass reshape \
  --tool "$FRONTEND/build-llvm18/buddy-framac-rewrite" \
  --llvm-prefix "$LLVM18" --frama-llvm-prefix "$LLVM18" --system-prefix "$SYS_PREFIX" \
  --compile-commands "$CASE_DIR/compile_commands.json" \
  --frama-c "$FRAMA_PREFIX/bin/frama-c" --frama-cirgen "$FRAMA_PREFIX/bin/framaCIRGen" \
  --output-dir "$CASE_DIR/result"
