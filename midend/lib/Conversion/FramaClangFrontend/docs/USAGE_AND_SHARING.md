# 当前前端的调用与 Gitee 分享说明

已验证版本：Clang/LibTooling **18.1.8**；Frama-Clang **0.0.20**（配套 Clang
**18.1.8**）；Frama-C **33.0**；Python **3.10+**；CMake **3.20+**、Ninja。
C++ 工具需要 LLVM/Clang 开发库和 `LLVMConfig.cmake`、`ClangConfig.cmake`。

## 本机直接运行

以下路径对应开发机；换机器需要按后文设置自己的工具链和编译数据库。

```bash
REPO=/home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git
FRONTEND="$REPO/midend/lib/Conversion/FramaClangFrontend"
MINI="$REPO/midend/lib/Conversion/MiniMLIR"

# 首次使用或 C++ 规则有修改时构建。
python3 "$FRONTEND/build_frontend.py" --jobs 1

# 一个完整 pass：改写 → 提取专用头文件 → 生成兼容副本 → 转换 → C 回读。
python3 "$FRONTEND/run.py" --pass builtin-transpose
```

运行全部九个 pass：

```bash
for pass_name in reshape matmul-scalar lower-ime silu-fusion \
                 assume-tight eliminate-copy gpu-memcpy depthwise-conv \
                 builtin-transpose; do
  python3 "$FRONTEND/run.py" --pass "$pass_name" || break
done
```

可选规则回归：

```bash
/home/ce/Desktop/BuddyCompiler/environment-v0.0.4/env/bin/ctest \
  --test-dir "$FRONTEND/build-llvm18" --output-on-failure
```

默认结果在 `MiniMLIR/current/<pass_name>/`。转换和回读全部通过才替换当前版本；
失败恢复上一份通过结果。`--output-dir` 仅用于需要保留诊断的全新/空目录，
`--skip-frama` 仅检查原生前端，不能作为转换通过结果。

## 上传哪些内容

保持两个目录在 Buddy 的 `midend/lib/Conversion/` 下相邻，保留原来的层次。

| 路径 | 内容 |
| --- | --- |
| FramaClangFrontend/include、src | C++ AST 分析、改写、成员提取 |
| FramaClangFrontend/experiment、tests | Python 编排、pass 配置、回归与参考输入 |
| FramaClangFrontend/CMakeLists.txt、build_frontend.py、run.py | 构建与完整调用入口 |
| FramaClangFrontend/docs、README.md、ARCHITECTURE.md | 原理、规则、边界与用法 |
| MiniMLIR/include | 唯一维护的完整抽象接口库 |
| MiniMLIR/python、extract.py、rewrite_headers.py、docs | 专用库生成、转换兼容规则与审计 |
| MiniMLIR/current/<pass_name> | 本次通过的原始/改写 pass、C、两份专用库和验证记录 |
| MiniMLIR/current/verification.json、logs | 九个 pass 的验证索引及汇总检查 |

每个 current 子目录的主要文件：

```text
original.cpp          改写前的完整 pass
rewritten.cpp         自动改写后的完整 pass
converted.c           转换后的 C
include/              从共享库按声明/成员提取的专用库
conversion-include/   从专用库生成的转换兼容副本
rewrite.patch / rewrite.json
header-rewrite.patch / header-rewrite.json
rewrite-inventory.json / summary.json
```

不上传本机 build 目录、编译数据库中的绝对路径缓存或工具链二进制。
前端 `.gitignore` 已排除 `build/`、`build-*/`、Python 缓存及历史构建产物。
`MiniMLIR/current/` 默认被忽略，作为本次分享的验证结果需要显式加入。

前四个 profile 仍读取历史 input.cpp/mini_mlir.h 进行对照和原件核对，必须连同
所需参考文件上传；正式转换默认使用共享抽象库，不会切回这些历史 mini。
Reshape 的 `final` 是相对符号链接，必须包含它指向的两个参考文件。

在当前 Buddy Git 仓库中，以下是定向加入分享内容的命令；它们不修改原始 pass。
执行前确认 REPO、FRONTEND、MINI 已设置。

```bash
cd "$REPO"

git add "$FRONTEND" "$MINI"

# 其余源码位于原 Buddy 源码树中，正常一并保留。
git add \
  midend/lib/Conversion/GraphRedundancyElimination/SimplifyTosaReshape.cpp \
  midend/lib/Conversion/GraphRedundancyElimination/SimplifyTosaMatmulScalar.cpp \
  midend/lib/Conversion/LowerIME/LowerIMEPass.cpp \
  midend/lib/Conversion/TosaElementwiseFusion/SiLUFusion.cpp \
  midend/lib/Conversion/AssumeTightMemRefLayout/AssumeTightMemRefLayout.cpp \
  midend/lib/Conversion/EliminateMemRefCopy/EliminateMemRefCopy.cpp \
  midend/lib/Conversion/MLIRGPU/ConvertMemcpyToGPU.cpp \
  midend/lib/Conversion/DepthwiseConvOptimization/DepthwiseConvNhwcHwc.cpp \
  midend/lib/Conversion/TransposeOptimization/BuiltinTransposeVectorization.cpp

# 当前入口所需的四份历史参考。
git add \
  midend/lib/Conversion/GraphRedundancyElimination/test_framaclang/simplify_tosa_reshape/full_pass_v2/final \
  midend/lib/Conversion/GraphRedundancyElimination/test_framaclang/simplify_tosa_reshape/full_pass_v2/steps/097_region_real_block_list_member/input.cpp \
  midend/lib/Conversion/GraphRedundancyElimination/test_framaclang/simplify_tosa_reshape/full_pass_v2/steps/097_region_real_block_list_member/mini_mlir.h \
  midend/lib/Conversion/GraphRedundancyElimination/test_framaclang/simplify_tosa_matmul_scalar/full_pass/steps/100_complete_pass/input.cpp \
  midend/lib/Conversion/GraphRedundancyElimination/test_framaclang/simplify_tosa_matmul_scalar/full_pass/steps/100_complete_pass/mini_mlir.h \
  midend/lib/Conversion/LowerIME/test_framaclang/steps/072_complete_pass/input.cpp \
  midend/lib/Conversion/LowerIME/test_framaclang/steps/072_complete_pass/mini_mlir.h \
  midend/lib/Conversion/TosaElementwiseFusion/test_framaclang/steps/104_arrayref_complete_pass/input.cpp \
  midend/lib/Conversion/TosaElementwiseFusion/test_framaclang/steps/104_arrayref_complete_pass/mini_mlir.h

# 只强制加入九个通过目录及验证索引，不加入根目录的 .lock 文件。
for pass_name in reshape matmul-scalar lower-ime silu-fusion \
                 assume-tight eliminate-copy gpu-memcpy depthwise-conv \
                 builtin-transpose; do
  git add -f "$MINI/current/$pass_name"
done
git add -f "$MINI/current/verification.json" "$MINI/current/logs"

git diff --cached --stat
git commit -m "Add Clang 18 Frama frontend and nine verified passes"

# 将下面的占位地址替换为你自己的 Gitee 仓库；已有 gitee remote 时不重复 add。
git remote add gitee https://gitee.com/YOUR_ACCOUNT/YOUR_REPOSITORY.git
git push -u gitee HEAD
```

这里只提供提交/推送命令，尚未替你执行 Git 加入、提交或推送。
如果另建独立展示仓库，仍应保留上述相对层次和参考文件；完整前端需在匹配的
Buddy 源码树中运行。仅复制两个目录可以审查实现和使用已生成结果，不能直接重跑
真实头文件解析。

## 别人的机器：构建完整前端

`build_frontend.py` 当前仍定位本开发工作区的 CMake/Ninja，换机器可直接调用 CMake。
先设置以下参数；不要照搬开发机的绝对路径：

```bash
REPO=/path/to/buddy-mlir
FRONTEND="$REPO/midend/lib/Conversion/FramaClangFrontend"
MINI="$REPO/midend/lib/Conversion/MiniMLIR"
LLVM18=/path/to/llvm18
FRAMA_PREFIX=/path/to/frama-install-prefix
SYS_PREFIX=/path/to/audited-conda-toolchain
COMPILE_DB=/path/to/local/buddy-build/compile_commands.json

export PATH="$LLVM18/bin:$FRAMA_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$LLVM18/lib:$FRAMA_PREFIX/lib/stublibs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

cmake -S "$FRONTEND" -B "$FRONTEND/build-llvm18" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$LLVM18/bin/clang" \
  -DCMAKE_CXX_COMPILER="$LLVM18/bin/clang++" \
  -DFRONTEND_LLVM_MAJOR=18 \
  -DLLVM_DIR="$LLVM18/lib/cmake/llvm" \
  -DClang_DIR="$LLVM18/lib/cmake/clang"
cmake --build "$FRONTEND/build-llvm18" --parallel 1

python3 "$FRONTEND/run.py" --pass builtin-transpose \
  --tool "$FRONTEND/build-llvm18/buddy-framac-rewrite" \
  --llvm-prefix "$LLVM18" \
  --frama-llvm-prefix "$LLVM18" \
  --system-prefix "$SYS_PREFIX" \
  --compile-commands "$COMPILE_DB" \
  --frama-c "$FRAMA_PREFIX/bin/frama-c" \
  --frama-cirgen "$FRAMA_PREFIX/bin/framaCIRGen"

ctest --test-dir "$FRONTEND/build-llvm18" --output-on-failure
```

完整前端运行还有两个环境前提：

1. 匹配版本的真实 Buddy/LLVM/MLIR 源码、生成头文件和本机编译数据库均已准备好。
   编译数据库需含所选 pass 的唯一条目，不能复用开发机记录的绝对路径。
   Clang 18 是前端/转换器版本，不代表把真实 MLIR API 降为 LLVM 18。
2. 当前 `experiment/toolchain.py` 对标准库路径采用已审计的 Conda 布局：
   `SYS_PREFIX/lib/gcc/x86_64-conda-linux-gnu/*/include/c++`，并使用
   `SYS_PREFIX/x86_64-conda-linux-gnu/sysroot`。
   使用普通发行版 `/usr` 工具链时，需要先扩展这层路径适配；仅把 SYS_PREFIX
   改成 `/usr` 尚不能运行。此限制在上传说明中应保留，不能宣称任意机器开箱即用。

## 只复现已有 pass 的 C 转换与回读

这一模式不运行前端改写/抽取，也不需要真实 MLIR 或 Buddy 编译数据库。
需要已生成的 `rewritten.cpp`、`conversion-include/` 及上述版本的转换工具。
下面以 Linux x86_64、无空格的路径为例；Frama 安装前缀含 `bin/frama-c` 和
`bin/framaCIRGen`。实际工具不在同一前缀时分别修改对应路径。

```bash
# 设置 REPO / MINI / LLVM18 / FRAMA_PREFIX；其他机器按实际安装位置填写。
CASE="$MINI/current/builtin-transpose"
OUT_DIR="$(mktemp -d)"
RESOURCE_DIR="$("$LLVM18/bin/clang++" -print-resource-dir)"

export PATH="$LLVM18/bin:$FRAMA_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$LLVM18/lib:$FRAMA_PREFIX/lib/stublibs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

"$FRAMA_PREFIX/bin/frama-c" "$CASE/rewritten.cpp" \
  -cxx-nostdinc -machdep gcc_x86_64 \
  "-cxx-clang-command=$FRAMA_PREFIX/bin/framaCIRGen" \
  "-fclang-cpp-extra-args=-std=c++17 -nostdinc++ -I$CASE/conversion-include -resource-dir=$RESOURCE_DIR -target x86_64-pc-linux-gnu" \
  -cxx-parseable-output -print -ocode "$OUT_DIR/converted.c"

"$FRAMA_PREFIX/bin/frama-c" "$OUT_DIR/converted.c" -machdep gcc_x86_64
```

只读回已有的 C，不需要 C++ 转换器：

```bash
"$FRAMA_PREFIX/bin/frama-c" "$CASE/converted.c" -machdep gcc_x86_64
```

精简转换命令在开发机对 Reshape、BuiltinTransposeVectorization 重新实测。
它验证已生成副本的转换，不替代完整前端回归，也不代表外部接口 ACSL 行为证明。
