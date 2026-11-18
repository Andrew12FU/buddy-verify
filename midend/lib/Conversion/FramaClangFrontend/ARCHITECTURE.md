# 前端代码审查与扩展指南

C++ 负责 AST 分析、兼容性改写和接口观察，Python 负责编排实验与先驱阶段的有界模型生成。当前支持 Reshape、MatmulScalar、LowerIME、SiLUFusion；完整 pass 的自动接口生成仍未实现；有界接口抽取先驱模块现已归入同一前端，见文末。

头文件与实现按相同职责目录组织：`frontend/` 为入口/配置/Clang 适配，`rewrite/` 为修改计划，`support/` 为 AST/源码及输出支持，`rules/` 为独立规则，`interface/` 为接口抽取和抽象分析。每条规则有自己的头文件，分发器显式包含各个规则接口；不再使用混合规则总头。实现不按 pass 复制，pass 特定范围由 profile 约束。

## 建议的审查顺序

不必从头阅读所有实现。先确认下面四个接口，再按感兴趣的规则进入实现。

1. `experiment/profiles.py`：本轮处理哪个 pass、使用什么参考、期待哪些修改。
2. `experiment/validation.py` 的 `ExperimentRunner.run()`：哪些检查必须通过，失败保存什么。
3. `include/rewrite/RewritePlan.h`：一项补丁、一次拒绝和最终接受分别意味着什么。
4. `include/rules/*.h`：规则的参数、作用、拒绝行为和返回约定。
5. `src/rules/WhileRule.cpp` 或 `src/rules/GuardRule.cpp`：先看较短的规则，再看捕获及命名空间。
6. 需要核对 Clang 生命周期和入口边界时，再读 `src/frontend/RewritePipeline.cpp` 和 `src/frontend/RewriteFrontend.cpp`。

公开 C++ 接口使用中文 Doxygen 风格注释；Python 使用中文 docstring 和类型标注。重点说明职责、参数单位、返回值、失败行为及借用对象生命周期。测试的 docstring 说明每个反例为何存在。

## C++ 层次

| 层 | 文件 | 负责 | 不负责 |
| --- | --- | --- | --- |
| 命令行 | `src/frontend/RewriteFrontend.cpp` | 参数、路径保护、串联调用、退出码 | AST 业务规则 |
| 配置策略 | `include/frontend/FrontendOptions.h` / `src/frontend/FrontendOptions.cpp` | profile、启用规则、include 白名单 | 编译数据库路径、原件快照 |
| Clang 适配 | `include/frontend/RewritePipeline.h` / `src/frontend/RewritePipeline.cpp` | 预处理回调、consumer/action、两遍 AST 扫描和规则分发 | 各条规则的匹配细节、文件输出 |
| AST/源码公共工具 | `include/support/AstSupport.h` / `src/support/AstSupport.cpp` | 主文件清单、局部事实、源码范围、命名和已知风险检查 | 选择哪些 guard 应改写 |
| 具体规则 | `include/rules/*.h`、`src/rules/*.cpp` | 前提判断、生成候选修改和拒绝原因 | 直接改 AST、写源码文件、调用 Frama-C |
| 内存修改计划 | `include/rewrite/RewritePlan.h` / `src/rewrite/RewritePlan.cpp` | 编辑去重、范围检查、冲突检测、合成文本 | 解析和后续转换验证 |
| 输出 | `include/support/FrontendOutput.h` / `src/support/FrontendOutput.cpp` | 路径身份比较、schema 1 JSON、文件写入 | 决定改写语义是否成立 |

```text
RewriteFrontend.main
  ├─ 解析参数 → FrontendOptions → 检查路径
  ├─ parseAndPlan
  │    ├─ 预处理：IncludeCollector 登记 include 输出补丁
  │    └─ 完成原始真实依赖解析
  │         ├─ 第一遍：collectMainFileInventory
  │         └─ 第二遍：RuleDispatcher
  │              ├─ NamespaceRule
  │              ├─ planWhileRewrite
  │              ├─ planCaptureRewrite
  │              ├─ planGuardRewrite
  │              ├─ planIfConditionRewrite
  │              └─ planValueRangeRewrite
  ├─ RewritePlan.finish：统一接受或拒绝
  └─ serializeReport / writeFile
```

嵌套捕获由 `src/rules/CaptureRule.cpp` 委托给 `src/rules/NestedCapture.cpp`，接口为 `include/rules/NestedCapture.h`。后者不是新增 CLI 规则：最外层 lambda 接管整个捕获树；按 canonical VarDecl 映射原对象；每层把继承指针复制到局部变量后再按值捕获；局部对象地址保留在其定义所在调用体内。所有内层编辑先做局部范围/冲突检查，再合成一个根替换，内层 AST 回调不重复提交。非嵌套样例继续走原路径。

此处确实扩展了旧规则范围，并定位到 Frama-Clang 对嵌套捕获字段的名称解析问题；适用范围、失败对照、返回闭包的生命周期验证与完整 pass 回归见 [NESTED_CAPTURE.md](docs/NESTED_CAPTURE.md)。

规则只通过 `RewriteContext` 借用 AST、配置、主文件清单和计划，不读取命令行全局变量。命名空间规则持有自己的名字映射，因为同一匿名空间的重开需要共享状态；其他规则没有跨文件状态。

`AstSupport` 中检查 operator&、未求值表达式和跳转，是现有规则的保守前提筛选，不是完整 ADL、控制流、逃逸或语义等价分析。注释不能被当成超出实现范围的正确性保证。

## 最容易误读的约定

**源码位置有两种范围。** Clang `SourceRange` 通常包含最后一个 token；计划 `add` 使用 `[Begin, End)` 的半开区间。规则要先调用 `tokenEnd` / `statementEnd`。`Offset/Length` 是 UTF-8 字节数，行列是 1 基审查位置，不能用 Python 字符串索引代替字节索引。

**所有补丁针对同一份原文。** 规则永远读 `Plan.Original`，不读某条先前规则的改写结果。`finish` 排序并拒绝重叠后，从后向前应用。零长度代表插入；普通同位置不同结果视为冲突。只有显式调用 `addInsertion` 的零长度插入可按不同的非负序号协作：数组/范围声明为 10，捕获指针为 20。同序号、插入与替换同起点、其他重叠仍拒绝；`rewrite.json.insertion_order` 保留顺序，普通编辑为 -1。

**三个结果不能混淆。**

| 情况 | 规则函数的动作 | 最终影响 |
| --- | --- | --- |
| 不匹配、未启用或已包块 | 直接返回，不加编辑 | 可以是正常无修改输入 |
| 匹配但当前前提不足 | `Plan.reject(...)` | 整份计划拒绝；不能输出较早登记的部分成功修改 |
| 匹配且前提满足 | `Plan.add(...)` | 仍须等待其他规则与 `finish` 的统一检查 |

规则函数返回 `void`，结果在计划中表达。Clang `Visit*` 返回的 `true` 只表示继续遍历，与上述任何一项成功判据都不等同。

**AST 指针只在当前解析期间有效。** `MainFileInventory`、`BodyFacts` 和 `RewriteContext` 借用 AST，不能跨轮保存。计划和报告只存文本、字节范围及理由；当前对象身份配对使用 canonical 声明，不将内存地址导出为稳定标识。

**include 替换不会影响本轮原始解析。** `IncludeCollector` 在预处理时登记补丁，Clang 仍读取真实库。最终写出的副本才使用已有 mini；这不是自动头文件提取。

**文件输出不是多文件原子事务。** 入口先拒绝已有输出/报告及指向原件的路径，再写报告，接受后才写源码。JSON 的 `rewritten` 表示内存计划接受；写入失败会返回 2，因此仍必须看进程退出码。参数错误通常不会产生报告。

## Python 层次

当前解析器和外部 Frama-Clang 转换器均默认使用各自配套的 Clang/LLVM 18；Frama-C 33.0 和 OCaml 插件保留，`framaCIRGen` 可独立选择。`toolchain.py` 分别生成原生与转换参数/进程环境，动态查询各自内建头目录。sysroot 和 GCC 标准库通过独立 `system_prefix` 保持一致。解析器与原生编译器、转换器与转换参数环境分别检查主版本一致性。构建入口为 `build_frontend.py`，保留旧 `build/`，默认使用 `build-llvm18/`。详见 [frontend 迁移](docs/CLANG23_MIGRATION.md)和 [转换器 23 历史实验](docs/FRAMA_CLANG23_MIGRATION.md)。

| 文件 | 对外入口 | 负责 |
| --- | --- | --- |
| `run.py` | `parse_config` / `main` | 参数、必需文件和非空目录检查、创建运行配置 |
| `experiment/profiles.py` | `PassProfile` / `RunConfig` | 已审计 pass 的来源、启用规则、计数和参考验收策略 |
| `experiment/toolchain.py` | `compiler_arguments` / `run_command` | 固定工具链参数、独立子进程环境、无 shell 调用、完整日志 |
| `experiment/artifacts.py` | 快照、对照输入、真实 include 恢复、哈希函数 | 产物重放与审计，不解释 C++ 语法 |
| `experiment/validation.py` | `ExperimentRunner.run` | 阶段顺序、失败状态、成功验收 |

```text
parse_config → ExperimentRunner 初始化
  → clang_version
  → rewrite（含 profile、命中数、人工参考检查）
  → native_checks（原始 mini / 改写 mini / 改写真实依赖）
  → idempotence_check
  → frama_checks（可显式跳过）
  → finish（原件保护、产物哈希和最终状态）
```

命令通过参数数组执行，日志中的命令字符串只用于阅读。非零退出返回阶段记录，不直接抛进程异常，因为原始 mini 转换失败是需要保存的对照。必需阶段失败由流程层转成 `failed`；超时记为退出 124。`--skip-frama` 只能得到 `native_checked`，不能得到 `converted`。

初始化失败可能发生在摘要建立之前，入口会报告初始化错误；已进入阶段的错误则保存已完成阶段。当前固定参数适用于已审计的 Conda x86_64 环境，不能把 `compiler_arguments` 看成通用编译选项重写器。

## 后续应在哪里扩展

| 想增加的能力 | 应修改的位置 | 需要一起审查的内容 |
| --- | --- | --- |
| 新 pass，复用现有规则 | Python `profiles.py`；C++ `FrontendOptions` 和相应 profile 匹配 | 独立原始输入、mini、命中范围、反例和完整转换 |
| 新改写规则 | `include/rules/*.h`、一个新的 `src/rules/*Rule.cpp` 及对应 `include/rules/*Rule.h`、分发和规则名登记 | 适用/拒绝条件、依赖变化、冲突、幂等性、测试；同时加入 CMake 源列表 |
| 新验证步骤 | `ExperimentRunner` 的独立阶段方法 | 何时运行、失败是否阻断、日志与状态边界 |
| 新报告字段 | `FrontendOutput`、Python 消费方 | schema 兼容性和原始字节位置的含义 |
| 自动接口提取 | 在 Clang 解析层增加独立分析能力 | AST 生命周期、真实/mini 绑定、依赖闭包和改写后的新增需求 |

接口提取独立于捕获/guard 规则。`include/interface/MiniSlice.h`、`src/interface/MiniSlice*.cpp` 在共享 MiniMLIR 上解析改写后的 pass，计算声明/成员闭包并输出字节删除计划；相邻 `MiniMLIR/python/emission.py` 仅将共享文件裁剪为专用库。共享模型与生成物的边界、结构及重载保留规则见 [MiniMLIR](../MiniMLIR/docs/EXTRACTION.md)。`experiment/interface_pilot/` 的真实 LLVM 粒度试验保留。

共享模型的外部实现抽象先于成员提取：参数、返回值、约束和必要类型结构保留，
外部行为后续用 ACSL 描述。`function_ref` 不导入存储和回调转发算法，局部 lambda
构造的可见适配体独立标记。抽象调整来源由 MiniMLIR 的
`docs/interface-abstractions.json` 维护，`emission.py` 检查哈希并保存每轮快照，
`rewrite-inventory.json` 引用它；不混入转换副本的兼容改写计数。

头文件转换兼容性单独放在 `MiniMLIR/python/rewrites/`：`pipeline.py` 管理只读输入、
规则顺序及新副本；`typed_attr.py` 负责有界模板特化展开。入口为
`MiniMLIR/rewrite_headers.py`，由 Runner 的 `rewrite_headers()` 阶段调用。该阶段
不会修改 `include/` 原提取库，也不写 Buddy 源码；输出为 `conversion-include/`。
实际规则、拒绝边界及报告字段见 [头文件改写](../MiniMLIR/docs/HEADER_REWRITES.md)。

## 验证记录

重构回归和产物对照见 [VALIDATION_REFACTOR.md](VALIDATION_REFACTOR.md)。第一、二轮记录保留原样，描述当时的单文件实现及产物，不回写为新历史。

## 本轮规则复用中的不适配

SiLU 的 lambda 在变量初始化中，旧捕获规则会拒绝；SiLU 的 guard 返回值在块内，旧 guard 规则不会命中。这两处只在 SiLU profile 扩展，旧 profile 的边界由回归锁定。同位置前置声明通过计划层的显式顺序组合，未普遍放宽重叠编辑。

详细条件、样例、完整回归产物见 [IME_SILU.md](docs/IME_SILU.md)。这类新输入与旧规则的不适配今后应独立记录，不能只调整期望计数后宣称复用成功。

## 接口抽取与抽象层

接口抽象和兼容性改写属于同一个前端，共用顶层 CMake 和 LLVM/Clang 配置。
当前默认 `build-llvm18/`，历史 `build/` 保留。改写、先驱探针和共享库提取分别有入口，
生产回归仅将共享库成员提取接入改写流程，先驱试验不改变模型。

| 路径 | 作用 |
|---|---|
| `include/interface/Probe.h`、`Analysis.h` | 无 AST 指针的观察结果、分析入口和借用约定 |
| `src/interface/ProbeMain.cpp`、`Probe.cpp` | 接口探针 CLI、预处理/AST 生命周期适配 |
| `src/interface/Inventory.cpp` | 主文件调用、实际重载、构造、默认参数、类型性质、override 观察 |
| `src/interface/LibrarySlice.cpp` | 两个真实 LLVM 类型的有界源码切片和短体依赖边 |
| `experiment/interface_pilot/python/` | 当前粒度策略、有界头生成、工具调用和对照矩阵 |
| `experiment/interface_pilot/fixtures/` | 真实 LLVM 接口的最小调用输入 |
| `experiment/interface_pilot/docs/` | 设计、实际失败/成功经验和待讨论策略 |
| 接口先驱临时目录 | 默认检查后清理；显式输出目录才保留诊断 |

这些观察暂不复用改写的 `AstSupport`，工具链参数准备也暂分别维护；目录整合并不表示分析实现已合并或语义契约已经建立。抽取策略推广前，先通过真实/模型绑定和粒度试验决定哪些公共能力适合共享。

迁移与回归见 [INTERFACE_LAYOUT.md](docs/INTERFACE_LAYOUT.md)。

## 结果保留策略

`experiment/output.py` 管理固定的 `MiniMLIR/current/<pass>/`。每个 pass 仅保留一份通过版本；运行期间暂存上一份，成功后移除暂存，失败或原生检查后恢复。显式 `--output-dir` 才保留额外诊断文件，既有非空显式目录仍拒绝覆盖。嵌套捕获与接口先驱检查默认使用自动清理的临时目录。

`experiment/artifacts.py::compact_verified_artifacts` 负责成功结果的内容精简：关键转换/回读日志集中到各 pass 的 `logs/`；版本合入摘要，常规成功日志删除。真实依赖检查在共享模式复用 `rewritten.cpp`。重复改写和原始转换对照所需的临时源码在成功后删除，检查结论与临时文件哈希保留在摘要中。失败运行不调用此精简步骤。
