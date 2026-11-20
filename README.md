# Frama-Clang 改写前端：独立仓库审查版

这是从 Buddy 工作区拆出的 C++ / Python 前端及九个 pass 的资料包。目录可整体搬迁，不需要放回 Buddy 的 `midend/lib/Conversion`。当前版本为独立复现与审查版，远程仓库为 [Andrew12FU/buddy-verify](https://github.com/Andrew12FU/buddy-verify)。本机可直接复制执行的命令见 [LOCAL_REPRODUCTION.md](review/LOCAL_REPRODUCTION.md)。

## 三部分内容

```text
.
├── 01_frontend/
│   ├── FramaClangFrontend/       # C++ AST 规则、Python 调度、测试与示例
│   ├── MiniMLIR/                 # 共享 mini 头文件、按 pass 提取及头文件兼容改写
│   ├── ExtractMiniMLIR/          # 独立的真实 AST 接口骨架提取实验工具
│   └── historical_interface_pilot/ # 早期探索代码，只作历史资料
├── 02_passes/<pass>/
│   ├── original.cpp             # 当前九个原始 Buddy pass，未做源码改写
│   ├── rewritten.cpp            # 当前默认规则的前端自动改写结果
│   ├── include/                 # 对应该 pass 的提取 mini 头文件
│   ├── conversion-include/      # 对应该 pass 的转换兼容头文件
│   └── provenance/              # 对应版本的改写、头文件提取、转换记录及 C 产物
├── 03_manual_passes/
│   ├── buddy_four/<pass>/       # 原先四份人工 rewritten.cpp + 各自 mini_mlir.h
│   ├── four_pass_clang18/       # 桌面 four_pass(1).zip 的源码及完整 include/ 库
│   └── builtin-transpose/      # 第九个人工版本及其 mlir/、llvm/ mini 库
├── review/                     # 适配差异、来源清单和本次验证记录
├── scripts/check_package.py    # 检查资料包文件完整性
├── build.py                    # 与当前工作目录无关的构建入口
└── run.py                      # 与当前工作目录无关的运行入口
```

`03_manual_passes` 中的源码和头文件按原字节保存，包括原换行格式；没有用当前自动结果替换历史人工版本。`02_passes` 和 `03_manual_passes` 是两套独立资料。`provenance/` 中的绝对路径是生成时的证据，不是独立版的运行配置。EliminateCopy 的结果已随源码 constructor 默认关闭而更新；其余八份源码保持。

| CLI pass 名 | 原 Buddy 文件 | 人工版本目录（03_manual_passes 下） |
| --- | --- | --- |
| reshape | SimplifyTosaReshape.cpp | buddy_four/reshape |
| matmul-scalar | SimplifyTosaMatmulScalar.cpp | buddy_four/matmul-scalar |
| lower-ime | LowerIMEPass.cpp | buddy_four/lower-ime |
| silu-fusion | SiLUFusion.cpp | buddy_four/silu-fusion |
| assume-tight | AssumeTightMemRefLayout.cpp | four_pass_clang18/AssumeTightMemRefLayout |
| eliminate-copy | EliminateMemRefCopy.cpp | four_pass_clang18/EliminateMemRefCopy |
| gpu-memcpy | ConvertMemcpyToGPU.cpp | four_pass_clang18/ConvertMemcpyToGPU |
| depthwise-conv | DepthwiseConvNhwcHwc.cpp | four_pass_clang18/DepthwiseConvNhwcHwc |
| builtin-transpose | BuiltinTransposeVectorization.cpp | builtin-transpose |

## 环境与构建

需要 Linux、Python 3.10+、CMake 3.20+、Ninja，以及带开发头文件和 CMake 配置的 LLVM/Clang 18。LLVM 安装中应包含 `clang`、`clang++`、`libclang-cpp`、LLVM 库和 `lib/cmake/{llvm,clang}`。本机验证使用 LLVM/Clang 18.1.8；不随资料包分发工具链二进制。

Python 使用标准库，不需要安装额外 pip 包。Frama-C / Frama-Clang 仅在要求转换为 C 时需要：本次验证使用已有的 Frama-Clang 0.0.20 / Clang 18 配套转换器。`framaCIRGen` 应与转换阶段选择的 LLVM 主版本一致。

在仓库根目录运行，替换工具链路径：

```sh
export LLVM_PREFIX=/path/to/llvm18
python3 build.py --llvm-prefix "$LLVM_PREFIX" --jobs 2
```

默认构建到 `01_frontend/FramaClangFrontend/build-llvm18/`，重新生成 CMake 配置，不使用原 Buddy 的构建缓存。CMake、Ninja 默认从 PATH 查找；不在 PATH 时可分别传 `--cmake /path/to/cmake --ninja /path/to/ninja`。如果指定其他 `--build-dir`，运行时同时用 `--tool /that/build/buddy-framac-rewrite` 选择对应工具；`buddy-mini-slice` 应位于同一目录。

```sh
ctest --test-dir 01_frontend/FramaClangFrontend/build-llvm18 --output-on-failure
python3 scripts/check_package.py
```

## 备用构造规则的状态

**当前九个 pass 均不启用源码 `constructor` 规则。**它的实现、显式 `--rules` 入口和专门测试保留；当前输出保留原有继承构造 using，不增加该规则生成的转发构造。历史 Frama-Clang 对实际触发的 `CXXInheritedCtorInitExpr` 有兼容限制；当前声明型 mini 流程未触发这种 pattern 构造，不能把潜在风险描述为这九个 pass 已出现的错误。未来输入或模型真正构造对象时，先定位诊断，再决定是否显式启用。具体原因、启用方式及边界见 [备用构造规则说明](01_frontend/FramaClangFrontend/docs/OPTIONAL_CONSTRUCTOR_RULE.md)。

MiniMLIR 头文件阶段的其他构造兼容规则独立保留；它们不是这里关闭的源码规则。

## 运行内置九个 pass

默认 `--parse-mode model` 使用随包共享 MiniMLIR，无需完整 Buddy 工程和编译数据库。它依次运行源码规则、专用 mini 库提取、语法检查、头文件兼容改写和幂等性检查。模型只提供已审计的有界接口；此模式不验证真实 MLIR/Buddy 依赖，也不链接运行完整 pass。

先仅执行原生检查：

```sh
python3 run.py --pass matmul-scalar \
  --llvm-prefix "$LLVM_PREFIX" \
  --skip-frama --output-dir results/matmul-native
```

再要求转换和独立 C 回读：

```sh
python3 run.py --pass matmul-scalar \
  --llvm-prefix "$LLVM_PREFIX" \
  --frama-c /path/to/frama-c \
  --frama-cirgen /path/to/framaCIRGen \
  --output-dir results/matmul-converted
```

将 `--pass` 换成上表其他名字即可使用其他内置输入。默认 LLVM 前缀来自 `LLVM_PREFIX` 或 PATH 中的 `llvm-config-18` / `llvm-config`；默认 Frama 工具来自 `FRAMA_C`、`FRAMA_CIRGEN` 或 PATH。转换阶段默认沿用 `--llvm-prefix`，也可用 `--frama-llvm-prefix` 或 `FRAMA_LLVM_PREFIX` 单独指定。

输出目录必须为新目录或空目录；已有结果不会被覆盖。程序不再隐式更新 Buddy 的 `MiniMLIR/current`。运行入口使用自身位置定位代码和资料，可从任意工作目录使用绝对路径调用。

## 读取自己的输入

```sh
python3 run.py \
  --input 01_frontend/FramaClangFrontend/examples/custom_input.cpp \
  --rules namespace,guard \
  --llvm-prefix "$LLVM_PREFIX" \
  --skip-frama --output-dir results/custom
```

`--input` 直接读取指定文件。`--rules` 决定启用的规则，支持：`namespace,constructor,const-local,range-for,while,if-condition,guard,init-list,value-range,capture`。组合顺序应结合源码结构选择；需要分轮的规则由调度器重新解析并在支持范围内迭代。

传入 `--input` 或 `--rules` 后，不再套用内置样例的固定编辑计数和参考文件检查；AST 拒绝条件、语法检查、提取检查和幂等性检查仍执行。仅指定 `--input` 时继承所选 `--pass` 的规则集合，建议像示例一样明确传 `--rules`。C++ 工具仍使用 `--pass` 选择已有配置标识，不会据此生成人工结果。

模型模式支持自包含输入以及随包 mini 库拥有的头文件。完整流程仍以有界 MiniMLIR 接口为前提；任意新第三方库或完整 MLIR API 需要相应模型，不能通过增加 include 路径自动获得语义模型。

## 使用真实工程头文件

真实依赖仍来自你自己的 LLVM/MLIR/Buddy 工程。不会把大型 LLVM 构建目录打进本包。指定真实工程的 `compile_commands.json`：

```sh
python3 run.py --pass matmul-scalar --parse-mode real \
  --compile-commands /path/to/build/compile_commands.json \
  --database-source /path/to/buddy/midend/lib/Conversion/GraphRedundancyElimination/SimplifyTosaMatmulScalar.cpp \
  --llvm-prefix "$LLVM_PREFIX" \
  --skip-frama --output-dir results/matmul-real
```

`--database-source` 是数据库中原文件的键，用于提取编译参数；实际改写输入仍是本包的 `02_passes/.../original.cpp`。如果 `--input` 本身就是数据库登记的文件，可以省略 `--database-source`。数据库中的相对 `file` 按条目的 `directory` 解析；编译输出和依赖文件选项会移除，避免产生原工程对象文件。

标准头文件默认由所选编译器查找。仅当使用原实验那种 Conda GCC/sysroot 布局时，显式加 `--system-prefix /path/to/that/prefix`；不会再强制使用某台机器的 sysroot。可重复传 `--extra-include /path/to/include` 或 `--extra-arg=-DNAME=value` 补充参数。真实工程自身的源码、生成头文件和数据库目录必须存在。

此模式增加改写代码的真实依赖语法检查，然后仍通过随包 MiniMLIR 提取/兼容库进行转换；摘要中 `real_dependencies_checked` 为 true。去掉 `--skip-frama` 并指定两个 Frama 工具，即可要求转换和回读。

## 输出与失败诊断

主要产物如下：

| 文件/目录 | 含义 |
| --- | --- |
| original.cpp / rewritten.cpp | 原始输入快照 / 本轮自动改写结果 |
| rewrite.json / rewrite.patch | 每轮源码编辑记录 / 完整源码差异 |
| include/ / conversion-include/ | 提取接口 / 转换兼容接口 |
| interface-plan.json / interface-manifest.json | 依赖选择和提取来源 |
| header-rewrite.json / header-rewrite.patch | 独立的头文件兼容改写记录 |
| summary.json | 状态、执行阶段、验证范围、指纹和产物哈希 |
| converted.c | 转换成功后生成的 C；`--skip-frama` 不生成 |
| logs/ | 转换、回读及失败阶段诊断 |

`native_checked` 表示只完成原生检查，`converted` 表示还通过转换与独立 C 回读；两者都不表示已经证明一般 C++ 语义等价。真实依赖是否检查应看 `parse_mode` / `real_dependencies_checked`，不能只看 `converted`。

成功阶段的一部分临时文件和重复诊断会清理，命令和退出码保留在摘要中。失败时保留已有证据，检查 `summary.json` 的 `error` 及相应日志。CLI 参数错误返回 2，阶段失败返回 1，所要求阶段全部通过返回 0。新的尝试请使用新的输出目录。

## 单独查看资料中的 pass

`02_passes` 中的原始和改写源码可用其 `include/` 进行原生语法检查，例如：

```sh
"$LLVM_PREFIX/bin/clang++" -std=c++17 \
  -I02_passes/eliminate-copy/include \
  -fsyntax-only 02_passes/eliminate-copy/rewritten.cpp
```

人工版本的头文件应与各自源码配套：

```sh
"$LLVM_PREFIX/bin/clang++" -std=c++17 \
  -I03_manual_passes/buddy_four/reshape \
  -fsyntax-only 03_manual_passes/buddy_four/reshape/rewritten.cpp

"$LLVM_PREFIX/bin/clang++" -std=c++17 \
  -I03_manual_passes/four_pass_clang18/include \
  -fsyntax-only 03_manual_passes/four_pass_clang18/EliminateMemRefCopy/rewritten.cpp

"$LLVM_PREFIX/bin/clang++" -std=c++17 \
  -I03_manual_passes/builtin-transpose \
  -fsyntax-only 03_manual_passes/builtin-transpose/rewritten.cpp
```

这里的 mini 是验证用接口，并非完整 MLIR 实现；它们不提供完整 pass 的链接/执行环境。

## 原始单头文件复现：特别区分 Reshape V1 / V2

`run.py` 的默认 model 流程使用共享 MiniMLIR，并生成 conversion-include，不等于使用历史单头文件。用下面这个单独入口，才能复现“前端输出 + 未修改的原 mini_mlir.h”。它只在新 input.cpp 中替换 include，不再次运行源码规则，也不改写头文件。

Reshape 的两版原头文件均已保留：

| 版本 | 包内路径 | 头文件行数 | 当前前端输出转换的 C 行数 |
| --- | --- | ---: | ---: |
| V1 | 03_manual_passes/buddy_four/reshape/v1/mini_mlir.h | 270 | 1,859 |
| V2 | 03_manual_passes/buddy_four/reshape/mini_mlir.h | 1,818 | 8,513 |

`buddy_four/reshape/mini_mlir.h` 继续是初次打包时保留的 V2 原件；V1 是本次新增的独立副本。两者都没有修改内容。

先生成自己的前端输出，再配 V1 转换：

```sh
python3 run.py --pass reshape --llvm-prefix "$LLVM_PREFIX" \
  --skip-frama --output-dir results/reshape-frontend
python3 scripts/reproduce_original_mini.py --pass reshape --reshape-mini v1 \
  --source results/reshape-frontend/rewritten.cpp \
  --llvm-prefix "$LLVM_PREFIX" \
  --frama-c /path/to/frama-c --frama-cirgen /path/to/framaCIRGen \
  --output-dir results/reshape-v1-original-mini
```

省略 --source 时，使用包内 `02_passes/reshape/rewritten.cpp` 快照；`--reshape-mini v2` 改用 V2。两个输出目录都必须为新目录或空目录。

MatmulScalar、LowerIME、SiLUFusion 可分别使用 `--pass matmul-scalar`、`--pass lower-ime`、`--pass silu-fusion`，配各自 buddy_four 目录中的原始 mini_mlir.h，当前生成 C 分别为 2,086、1,192、3,027 行。该入口输出 input.cpp、原字节 mini_mlir.h、converted.c、summary.json 和完整阶段日志，核对头文件 SHA-256，并要求 Clang、转换及 C 回读全部通过。

本轮实测摘要见 [原始 mini 复现记录](review/ORIGINAL_MINI_VALIDATION.json)。行数包含空行和注释，适用于本次记录的工具链；不表示 V1 与 V2 的接口性质等价。

## 本次适配与审查入口

本次完成独立源码构建、15 组 CTest、九个模型模式完整转换与回读。关闭源码 constructor 后重新验证，当前结果与 `02_passes` 一致；EliminateCopy 相比初次打包仅移除了该规则生成的转发构造，其余八份源码逐字节不变。另通过真实工程 Matmul 完整流程、带空格路径搬迁、自定义输入、相对编译数据库，以及九个人工改写版本与其原 mini 头文件的语法检查。

- [适配变更说明](review/CHANGELOG.md)：路径、调度及一处构造 AST 默认参数读取的具体调整。
- [适配前后 patch](review/adaptation.patch)：相对于拆包初始源码的变化。
- [来源与哈希](review/source-manifest.json)：原 Buddy、历史人工源码和头文件的复制来源。
- [本轮验证记录](review/VALIDATION.json)：当前源码指纹、产物对比和验证结果。
- [包内文件清单](package-manifest.json)：本包源文件和资料的完整性校验。

桌面目录中的构建产物、适配前快照和完整运行日志仅供本机审查，已由 `.gitignore` 排除；源码压缩包也排除了这些本机产物。`docs/history/` 和 `historical_interface_pilot/` 保留原工程开发资料，旧路径属于历史记录，不是新的运行入口。当前维护入口是根目录 `build.py` / `run.py`，以及说明中明确列出的工具。
