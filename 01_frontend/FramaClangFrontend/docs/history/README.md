# Buddy 的 Frama-Clang 前端：源码改写与接口抽象

2026-11-20 源码规则通用化及逐规则备份/验证见 [剩余规则改写记录](docs/REMAINING_RULE_GENERALIZATION.md)。规则层不再按 profile 名称分支；下文旧 profile 限制以该记录和当前实现为准。

独立的 [ExtractMiniMLIR](../ExtractMiniMLIR/README.md) 正在开发从真实 AST 从零生成接口骨架的链路，
不使用下述共享 MiniMLIR 填补生成声明。简单案例已通过转换；TosaMatmul 候选仍有明确的模板等阻塞。

当前源码解析、AST 改写和接口抽取默认使用 **Clang/LibTooling 18.1.8**；最终 C++→C 转换使用 **基于 Clang 18.1.8 的 Frama-Clang 0.0.20**，由 Frama-C 33.0 驱动。普通 Clang 18 负责解析和检查，C++→C 仍由 Frama-Clang 执行。2026-11-18 按用户要求备份 23 前端后恢复 18；新增四个 pass 以 `tests/references/four_pass_clang18/` 中的 `four_pass(1).zip` 原始快照为基线。恢复与对齐结果见 [Clang 18 恢复记录](docs/CLANG18_RETURN.md)。[23 前端迁移](docs/CLANG23_MIGRATION.md)与 [23 转换器实验](docs/FRAMA_CLANG23_MIGRATION.md)仅作历史记录。

本目录包含同一个前端的两部分能力：**源码兼容性改写**与**接口抽取和抽象**。C++ / Clang LibTooling 负责 AST 分析，Python 负责实验调度和有界模型生成。四个 pass 默认从相邻 [MiniMLIR](../MiniMLIR/README.md) 的唯一共享模型中按声明/成员生成专用 include 库，源码保留原 include 路径。共享模型仍是经过人工审计的有限抽象；这不表示已经自动从任意真实 MLIR 库生成抽象模型。

接口抽象入口及实测经验见 [接口抽象先驱模块](experiment/interface_pilot/README.md)，本次统一目录的映射和验证见 [目录整合记录](docs/INTERFACE_LAYOUT.md)。

准备审查代码时，先读 [代码审查与扩展指南](ARCHITECTURE.md)：其中给出阅读顺序、层次边界、参数/返回值约定和扩展位置。公开接口和关键算法现已补充中文解释性注释，入口不再承载全部业务逻辑。

当前源码/头文件规则、四个 pass 的分别计数、额外不适配及下一步提取入口统一见
[改写清单](docs/REWRITE_INVENTORY.md)。每轮自动生成 `rewrite-inventory.json`，失败轮次也保留已应用的修改。

原始 pass、真实 LLVM/MLIR 头文件和已有人工实验均不修改。自动改写结果更新固定的当前通过目录。没有将工具加入父级 `Conversion/CMakeLists.txt`：前端开发库、Buddy 构建及外部 Frama-Clang 转换器分别配置。

实现文件采用对应的职责目录：头文件在 `include/`，实现文件在 `src/`，不混放。规则以语法能力命名，pass 的来源和验收范围集中在配置层。

```text
FramaClangFrontend/
  include/                    C++ 声明
    frontend/                 配置、改写解析流程
    rewrite/                  文本修改计划
    rules/                    按规则拆分的兼容性改写
    support/                  AST/源码工具、报告输出
    interface/                接口抽取、调用观察、有界分析的声明
  src/                        与 include/ 对应的 C++ 实现及两个工具入口
    frontend/  rewrite/  rules/  support/  interface/
  experiment/                 实验编排
    *.py                      改写的 pass 配置、工具链、产物、验证流程
    interface_pilot/          接口抽象先驱脚本、样例、经验、历史及新结果
  tests/                      改写规则行为和拒绝路径回归
  docs/                       前端结构、改写接入及迁移验证
  run.py                      完整 pass 改写回归入口
  CMakeLists.txt              构建改写工具、接口探针与共享库成员提取器
  build/                      两个工具共用的构建目录（忽略）
  ../MiniMLIR/current/<pass>/  每个 pass 唯一的当前通过结果（忽略）
```

**完整 pass + mini 是独立回归层**：四种 `--pass` 均处理完整文件，执行原生检查、Frama-Clang 转换及幂等性检查；另检查真实依赖下的原生解析。小用例不能替代完整文件回归。当前共享库回归及明确保留的转换障碍见 [MiniMLIR 回归记录](../MiniMLIR/docs/VALIDATION.md)。嵌套捕获记录见 [嵌套 lambda](docs/NESTED_CAPTURE.md)；历史结果仍保留。

调用命令、迁移依赖、Gitee 上传清单及已有结果的独立复现见 [使用与分享说明](docs/USAGE_AND_SHARING.md)。

## 当前实现

| 文件 | 职责 |
| --- | --- |
| `src/frontend/RewriteFrontend.cpp` | 仅负责 C++ 命令行、路径保护和顶层串联 |
| `FrontendOptions` / `RewritePipeline` | 配置与 Clang 回调、两遍 AST 扫描、规则分发 |
| `AstSupport` / `RewritePlan` / `FrontendOutput` | 源码/AST 公共工具、内存修改计划、JSON 和文件输出 |
| `include/rules/*.h` / `src/rules/*.cpp` | 独立规则的接口及实现，不直接修改 AST 或写文件 |
| `include/interface/*.h` / `src/interface/*.cpp` | 接口抽取探针、真实调用/类型观察及两个 LLVM 类型的有界切片 |
| `MiniSlice.h` / `MiniSlice*.cpp` | 共享模型上的声明/成员依赖闭包和裁剪计划 |
| `experiment/interface_pilot/` | 接口抽象实验编排、对照、经验、迁移映射和结果 |
| `run.py` | 仅负责改写回归的 Python 命令行、文件检查和运行配置 |
| `experiment/*.py` | pass 配置、编译数据库、进程、产物和分阶段验证 |
| `tests/rewrite_support.py` / `tests/test_*.py` | 行为对照、拒绝路径、幂等性及原文件保护测试 |
| `VALIDATION.md` | 第一轮 Reshape 的实测结果与产物位置 |
| `VALIDATION_MATMUL_SCALAR.md` | 第二轮 MatmulScalar 的实测结果及 Reshape 复测 |
| `ARCHITECTURE.md` / `VALIDATION_REFACTOR.md` | 分层审查指南与本轮重构回归 |

原有六类源码规则如下；四个新增 pass 的范围循环、构造、const-local 和列表规则见 [Clang 18 接入记录](docs/CLANG18_RETURN.md)：

1. 匿名命名空间具名化，并限定外部类类型引用。重开的同一匿名空间复用名称；命名冲突时选择新名称。**这改变内部链接属性，只用于单翻译单元转换副本。**
2. while 条件声明变为每轮声明、显式 bool 判断和原循环体。保留原循环体的嵌套块，避免改变遮蔽关系；`continue` 仍重新执行初始化。非平凡析构、初始化清理、goto/label、引用/volatile 条件变量及非复合循环体暂时拒绝。
3. `capture`：所有 profile 共用捕获树规划器。对象、标量、引用变量等的引用捕获改为原存储地址的值捕获，普通值捕获保留副本语义；同块表达式及单个自动局部声明均按 AST 结构接受。嵌套逐层传递地址，`this` 的普通整数字段访问统一转换为该实例的成员地址。初始化捕获、跨已改写引用的值捕获、泛型、宏、未求值使用及重载取地址等仍拒绝，不延长原对象寿命。详见 [通用 capture](docs/GENERIC_CAPTURE.md)。
4. `guard`：统一识别运行期 `if + 内建 ||`，不依赖 profile、类名、方法名、分支形状或返回值。块内 bool 保存完整原条件，then/else 原文完整保留；支持循环内、无花括号分支和 if 初始化语句。嵌套及 else-if 候选分轮重新解析，防止重叠并保持求值路径；宏、goto/label 拒绝，`if constexpr` 和重载 `||` 跳过。规则、当前命中数和验证范围见 [通用 guard](docs/GENERIC_GUARD.md)。

5. `if-condition`：SiLU、AssumeTight、EliminateCopy、GPU Memcpy 共用“分支补块 → fresh AST → 条件声明展开”的流程。无花括号的 then 和普通 else 先规范化，else-if 链保持直连；每个根每轮只规范化或展开之一。所有 profile 使用同一祖先检查，一般嵌套延后到后续轮次，直接 else-if 由根递归展开。右侧查询只在左条件为假时执行；宏、跳转、引用/volatile 条件变量、非平凡析构及可观察的临时清理拒绝。详见 [规范化与分轮说明](docs/IF_CONDITION_NORMALIZATION.md)。
6. `value-range`：SiLU 的 `GenericOp::create` 两个列表实参改用具名 `Value` 数组、指针/长度 `ArrayRef` 和 `ValueRange`。支持多个元素，数组覆盖原调用的生命周期。仅接受已审计的局部 Value/EmptyOp 元素及调用形状；空列表、有副作用实参、捕获初始化等情况拒绝。所选构造重载变化会写入报告。

`--interface-header` 保留为历史单头文件模式：先解析真实头文件，再替换已审计 includes。默认 runner 不启用此选项，而是保留 include 并调用 `buddy-mini-slice`。提取规则见 [MiniMLIR 提取说明](../MiniMLIR/docs/EXTRACTION.md)，原有真实 LLVM 先驱实验仍独立保留。

这些规则只在列明的四个 pass 和回归样本上验证，尚不是任意 C++ 的通用语义保持转换器。若规则相互覆盖、源范围不可编辑或发现不支持的候选，整次计划拒绝；以后可分轮执行，再接入其他 pass 的规则。

## 构建与回归

当前默认工具链为 `/home/ce/Desktop/framaclang/environment/llvm18` 中的 LLVM/Clang 18.1.8。它不改变 Buddy 的 LLVM/MLIR 23.0.0git 子模块。23 工具链、构建目录及切换前快照均保留。

```bash
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/build_frontend.py
/home/ce/Desktop/BuddyCompiler/environment-v0.0.4/env/bin/ctest \
  --test-dir /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/build-llvm18 \
  --output-on-failure
```

`build_frontend.py` 默认单任务，在 `build-llvm18/` 使用 Clang 18 编译并链接同套 18 开发库，拒绝覆盖历史 `build/`。若需试验 23，须同时显式指定 `--llvm-major 23 --llvm-prefix /home/ce/Desktop/BuddyCompiler/environment-v0.0.4/llvm23/root/usr/lib/llvm-23 --build-dir <另一个新目录>`，不覆盖原 `build-llvm23/`。

## 逐个运行 pass

```bash
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/run.py --pass reshape
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/run.py --pass matmul-scalar
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/run.py --pass lower-ime
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/run.py --pass silu-fusion
```

默认使用工作区 `build-v0.0.4/buddy/compile_commands.json` 中当前 pass 的条目。四个 pass 共用 `../MiniMLIR/include`，每轮根据改写后的 AST 生成自己的 `include/`；下面列出的旧 mini 只作为只读历史参考，显式传入 `--interface` 才启用历史模式：

| pass 参数 | 参考目录，相对于 Conversion | 启用规则 |
| --- | --- | --- |
| `reshape`（默认） | `GraphRedundancyElimination/test_framaclang/simplify_tosa_reshape/full_pass_v2/final/` | namespace、while、capture、guard |
| `matmul-scalar` | `GraphRedundancyElimination/test_framaclang/simplify_tosa_matmul_scalar/full_pass/steps/100_complete_pass/` | namespace、guard |
| `lower-ime` | `LowerIME/test_framaclang/steps/072_complete_pass/` | namespace |
| `silu-fusion` | `TosaElementwiseFusion/test_framaclang/steps/104_arrayref_complete_pass/` | namespace、if-condition、guard、value-range、capture |
| `assume-tight` | `FramaClangFrontend/tests/references/four_pass_clang18/AssumeTightMemRefLayout/` | namespace、capture、range-for、if-condition |
| `eliminate-copy` | `FramaClangFrontend/tests/references/four_pass_clang18/EliminateMemRefCopy/` | namespace、constructor、range-for、if-condition、guard |
| `gpu-memcpy` | `FramaClangFrontend/tests/references/four_pass_clang18/ConvertMemcpyToGPU/` | namespace、capture、range-for、if-condition |
| `builtin-transpose` | `FramaClangFrontend/tests/references/builtin-transpose/` | namespace、const-local、init-list、capture |
| `depthwise-conv` | `FramaClangFrontend/tests/references/four_pass_clang18/DepthwiseConvNhwcHwc/` | namespace、const-local、init-list、capture |

真实编译条目的语法检查调整、目标、sysroot 和标准库搜索参数全部记录。当前配置限定于已审计的 x86_64 Conda/GCC 环境，不是跨平台探测器。直接调用 C++ 工具时，通过同名 `--profile` 选择实验配置和 include 白名单；guard 和 capture 的 AST 匹配不依赖 profile；runner 会同步设置 profile 和启用规则，重复改写检查也使用同一配置。

常用参数：`--llvm-prefix`（解析/原生检查，默认 18）、`--frama-llvm-prefix`（转换器配套环境，默认 18）、`--frama-cirgen`（独立 C++ 转换器，默认原 OPAM 中的 18 版）、`--system-prefix`（已审计标准库/sysroot）、`--tool`、`--compile-commands`、`--interface`、`--frama-c`、`--output-dir`。若显式试用其他转换器，必须同时选择其配套 LLVM 前缀。`--skip-frama` 明确跳过转换，结果只能记作 `native_checked`。

默认更新 `../MiniMLIR/current/<pass参数>/`，每个 pass 只保留一份完整通过的源码和头文件库结果。转换及 C 回读通过后替换；失败或 `--skip-frama` 保留上一份通过版本，不累计快照。只有显式 `--output-dir` 才保留额外诊断目录，该目录必须全新或为空。

```text
original.cpp            原始源码快照
include/                从共享库按声明/成员提取的本轮专用库
interface-plan.json     AST 依赖闭包、声明范围及保留理由
interface-manifest.json 共享来源、删除清单和生成头文件哈希
interface-abstractions.json 共享模型外部接口抽象调整的快照及来源（独立于兼容规则）
conversion-include/     从专用库自动生成的转换用头文件副本
header-rewrite.json     头文件规则、模板来源、UTF-8 字节编辑和输入/输出哈希
header-rewrite.patch    头文件兼容改写的完整差异
rewrite-inventory.json  源码/头文件分别统计、完整编辑及后续提取的产物索引
rewritten.cpp           自动改写输入，保留原 include 路径
rewrite.patch           原始源码到自动结果的完整差异
rewrite.json            规则、原始字节偏移/行列、前后文本、依据及依赖变化
converted.c            本次成功转换的 C
logs/                  原始转换对照、改写转换与 C 回读的完整日志
summary.json           各阶段独立结果、输入/工具/实现/产物哈希和限制
```

成功检查后自动删除 `baseline_mini.cpp`、`second.cpp/second.json` 等检查副本。共享模式的真实依赖检查直接使用 `rewritten.cpp`，不再复制 `rewritten_real.cpp`。幂等性结论、工具版本和所有阶段的命令/退出码保存在 `summary.json`；普通成功阶段的重复日志删除。显式输出目录中的失败运行保留已有诊断，统一在 `logs/`。

`rewrite.json` 的偏移以原始 UTF-8 文件的字节为单位；全部修改都针对同一原始缓冲区。保留的前后文本与顺序可用于后续建立源码映射、分析依赖变化，不导出不稳定的 AST 内存地址。

summary 分别记录两套工具链路径、原生检查参数和 Frama-Clang 参数；JSON 中的解析器主版本必须与原生编译器一致，否则停止，防止误用旧二进制。

默认顺序为：真实依赖解析和改写 → 共享库成员提取 → 原始/改写源码在专用库下检查 → 真实依赖检查 → 独立头文件兼容改写及原生检查 → 幂等性 → Frama-Clang 原始对照及改写转换 → 生成 C 的独立回读。最终转换使用 `conversion-include/`。头文件规则及审查入口见 [HEADER_REWRITES.md](../MiniMLIR/docs/HEADER_REWRITES.md)。原始对照的失败只记录；其余阶段失败不会记作完整通过。保留真实继承构造导致的已知转换阻塞记作 `conversion_blocked`，日志和两份专用库照常保存。

四个 pass 均要求规则命中数量与审阅过的结构一致；共享模式只从期望计数中移除 include 替换项。MatmulScalar 严格比较已审阅的通用 guard 参考（旧人工版本保留）：共享模式先恢复参考文件的原 include 块，然后逐字节比较其余输出；历史模式直接比较原参考。其他三个 pass 保留已记录的表达形式或空白差别，不强制参考完全一致。新增 pass 应先建立自己的证据和命中范围。

## 接口抽取和抽象的当前边界

接口分析与兼容性改写使用独立 Clang action。当前已接入人工审计的共享 MiniMLIR 模型上的声明/成员提取，保留结构和重载依赖；它不会自动从任意真实 MLIR 库构造抽象模型。先驱模块对真实调用、类型性质、LogicalResult 短实现和 ArrayRef 成员的试验继续保留，供后续扩展抽象模型参考。

新增规则需要同时说明适用前提、拒绝条件、依赖变化和证据；用现有最小失败与成功对照回归，再放回完整 pass。接口模型修正应单独记录，不能隐藏在 Buddy 源码改写中。

原生检查不验证链接、执行或完整接口绑定等价。Frama-Clang 退出 0 且生成非空 C 只表示本次模型和配置下可转换；不代表真实 MLIR 实现已转换、一般语义等价或形式化证明完成。保留的转换日志包含现有接口模型的警告。

接口抽象先驱实验使用同一次构建生成的 `build-llvm18/header-extraction-probe`：

```bash
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/experiment/interface_pilot/run.py
```

BuiltinTransposeVectorization 的 Clang 18 头文件对照、源码规则、旧规则不适配与外部接口抽象边界见 [接入记录](docs/BUILTIN_TRANSPOSE.md)。
