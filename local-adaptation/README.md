# 桌面仓库本地适配：Reshape

本次取用 GitHub `Andrew12FU/buddy-verify` 的 `main`，提交
`e7e8cacf4710be4de7f0353c417dec4699f9acbd`。
桌面工作副本：`/home/ce/Desktop/buddy-verify`。

## 本次下载方式

本机命令行没有 GitHub 登录凭据，直接 `git clone` 返回认证错误。因此通过已授权的
GitHub 连接器读取 `main`、两个原始提交和完整文件树，复用本机上传缓存中 Git blob
校验值相同的文件，恢复 Git 对象及工作副本。本次共核对
1523 个文件/符号链接，提交、文件树、历史均与远端一致，`git fsck --full` 通过。
`origin` 已配置为 `https://github.com/Andrew12FU/buddy-verify.git`。

其他机器可以使用 Git 下载；若仓库为私有，需先配置具有访问权限的 Git 凭据：

```bash
cd "$HOME/Desktop"
git clone https://github.com/Andrew12FU/buddy-verify.git
cd buddy-verify
python3 verify_snapshot.py
```

## 必要环境与本次适配范围

- 前端：Clang/LibTooling 18.1.8，从本次下载的源码独立构建。
- 转换：Frama-Clang 0.0.20（配套 Clang 18.1.8）。
- C 回读：Frama-C 33.0。
- 外部真实依赖：开发机现有的完整 Buddy/LLVM/MLIR 源码、生成头文件和编译数据库。
- 本仓库保留抽象接口库及 pass，但不包含整个真实 Buddy/LLVM 工程。

本轮不修改 pass、前端规则或共享头文件。只新增本地调用脚本及一条重定位后的编译记录。
编译记录中的 `file` 和唯一的源码参数换为桌面源码；`directory`、真实 `-I` 路径仍指向
现有完整工程。不能把编译数据库中的所有旧路径一并替换成精简仓库路径。
脚本会要求外部工程的 Reshape 源码与下载源码逐字节一致，避免套用其他版本的环境。

## 完整运行脚本

仓库已包含本轮通过结果。需要重跑时，先清空这一份固定结果，再运行脚本：

```bash
# 仅删除本轮生成结果，保留脚本、源码和共享库。
rm -r -- /home/ce/Desktop/buddy-verify/local-adaptation/reshape/result
bash /home/ce/Desktop/buddy-verify/local-adaptation/run-reshape.sh
```

脚本依次执行：构建两个工具 → 重定位编译记录 → 改写 pass → 按声明/成员提取头文件
→ 生成转换兼容副本 → 原生检查 → C++ 转 C → Frama-C 回读。
首次运行的 `local-adaptation/reshape/result/` 必须不存在或为空。
每次保留这一份结果；需要重跑时先自行清空这个固定结果目录。

## 展开后的本机命令

```bash
REPO=/home/ce/Desktop/buddy-verify
WORKSPACE_ROOT=/home/ce/Desktop/BuddyCompiler
FRONTEND="$REPO/midend/lib/Conversion/FramaClangFrontend"
CASE_DIR="$REPO/local-adaptation/reshape"
LLVM18=/home/ce/Desktop/framaclang/environment/llvm18
SYS_PREFIX="$LLVM18"
FRAMA_PREFIX=/home/ce/.opam/framac-study
CMAKE="$WORKSPACE_ROOT/environment-v0.0.4/env/bin/cmake"
NINJA="$WORKSPACE_ROOT/environment-v0.0.4/env/bin/ninja"
COMPILE_DB="$WORKSPACE_ROOT/build-v0.0.4/buddy/compile_commands.json"

export PATH="$LLVM18/bin:$FRAMA_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$LLVM18/lib:$FRAMA_PREFIX/lib/stublibs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

"$CMAKE" -S "$FRONTEND" -B "$FRONTEND/build-llvm18" -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$NINJA" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$LLVM18/bin/clang" \
  -DCMAKE_CXX_COMPILER="$LLVM18/bin/clang++" \
  -DFRONTEND_LLVM_MAJOR=18 \
  -DLLVM_DIR="$LLVM18/lib/cmake/llvm" \
  -DClang_DIR="$LLVM18/lib/cmake/clang"
"$CMAKE" --build "$FRONTEND/build-llvm18" \
  --target buddy-framac-rewrite buddy-mini-slice --parallel 1

python3 "$REPO/local-adaptation/prepare_compile_commands.py" \
  --repo "$REPO" --database "$COMPILE_DB" \
  --output "$CASE_DIR/compile_commands.json"

python3 "$FRONTEND/run.py" --pass reshape \
  --tool "$FRONTEND/build-llvm18/buddy-framac-rewrite" \
  --llvm-prefix "$LLVM18" --frama-llvm-prefix "$LLVM18" \
  --system-prefix "$SYS_PREFIX" \
  --compile-commands "$CASE_DIR/compile_commands.json" \
  --frama-c "$FRAMA_PREFIX/bin/frama-c" \
  --frama-cirgen "$FRAMA_PREFIX/bin/framaCIRGen" \
  --output-dir "$CASE_DIR/result"
```

此处显式指定编译数据库很重要：仓库移动后，前端默认值会推导到桌面下的
`build-v0.0.4/buddy/compile_commands.json`，那里没有真实构建记录。
`build_frontend.py` 仍使用开发工作区的默认布局；这里直接使用 CMake 参数配置新目录。

其他机器使用时，设置 `WORKSPACE_ROOT`、`LLVM18`、`FRAMA_PREFIX`、`SYS_PREFIX`、
`COMPILE_DB`、`CMAKE`、`NINJA` 后运行同一脚本。本轮只验证现有 Conda 标准库布局；
`SYS_PREFIX=/usr` 尚不能直接替代。真实 LLVM/MLIR 接口来源版本见根目录的
`provenance.json`，与解析器采用 Clang 18 是两件事。

## 本轮结果位置与检查

`local-adaptation/reshape/result/` 中保留本轮新生成的：

- `original.cpp`、`rewritten.cpp`、`converted.c`。
- `include/`：从下载仓库的共享库提取的 Reshape 专用接口。
- `conversion-include/`：专用库的转换兼容副本。
- 源码/头文件改写清单、补丁及 `summary.json`。
- `logs/`：每个阶段的精确命令、工作目录和退出码。

仓库原有 `MiniMLIR/current/` 九个 pass 的上传结果保持原样。可以继续运行
`python3 "$REPO/verify_snapshot.py"` 核对已上传文件。
本轮独立验证结论见 `local-adaptation/reshape/verification.json`。
这些检查确认解析、转换和 C 回读；没有进行 ACSL 行为证明。

本目录随仓库同步，包含调用脚本、环境适配记录及本轮验证结果。

## 本次实测结论（2026-11-18）

下载源码独立编译成功，Reshape 的正式改写、接口提取、原生检查、转换及 C 回读通过。
源码改写共 8 项，头文件兼容改写共 7 项；再次改写 0 项。没有新增改写规则，也没有
修改已上传的前端、原始 pass 或共享头文件。改写 C++ 与上传结果逐字节一致；生成 C
只有四处断言中的头文件绝对路径改变，替换该路径前缀后逐字节一致。

未改写源码的 mini 对照 `frama_baseline_mini` 退出 4，在 Frama-Clang 的
`reorder_defs` 阶段失败；上传记录中的同一对照也退出 4。这一对照失败不计为正式改写
流程通过。正式结果的 `frama_rewritten` 和 `frama_c_readback` 均退出 0。
各阶段命令和完整诊断见 `reshape/result/logs/`。
