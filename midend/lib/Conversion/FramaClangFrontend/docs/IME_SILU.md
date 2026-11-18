# LowerIME / SiLUFusion 自动改写接入

自动运行快照已按保留策略清理；本文保留历史发现，当前通过版本见 [MiniMLIR/current](../../MiniMLIR/current)。

2026-11-16。仅接入用户确认的 LowerIME、SiLUFusion，DAP / RVV / BUD 未纳入本轮。原始 Buddy 源码、历史人工实验、真实依赖和 mini 头文件均只读；本轮复用既有 mini，没有实现头文件提取。

## 目录与审查入口

`include/` 和 `src/` 下均细分 `frontend/`、`rewrite/`、`support/`、`rules/`，各自承载配置/解析、修改计划、公共支持和具体规则。六条规则各有独立头文件，取消混合规则总头；Python 继续分为配置、工具链、产物、验证流程。具体职责见 [ARCHITECTURE.md](../ARCHITECTURE.md)。构建源列表、所有 include 和递归源码哈希收集均已同步。

LowerIME 的新入口为 `run.py --pass lower-ime`：复用命名空间规则；替换十个 includes，命名一个匿名空间，并限定类外方法定义及注册处两个类型引用，共 13 项编辑。

SiLU 的新入口为 `run.py --pass silu-fusion`：替换十一个 includes；同一匿名空间的两次重开统一命名，并限定注册类型；展开一个 if/else-if 条件链；为一个直属 failure guard 包块；两个 ValueRange 列表替换为数组支持的范围；一个 lambda 按原对象指针捕获。共 21 项编辑，规则计数逐项验收。

## 额外汇报：旧规则在新输入中的不适配

以下是旧实现的匹配/拒绝条件与本轮输入的结构差别，不把它们记为“原规则无修改复用”。

| 复用对象 | 新输入与旧规则的差别 | 本轮处理 | 保留的边界及证据 |
| --- | --- | --- | --- |
| capture | 旧规则仅接受块内直接表达式语句；SiLU 的 lambda 是 `GenericOp fusedOp = ...` 初始化的实参，沿 AST 向上会遇到 VarDecl，旧实现拒绝 | **仅 SiLU profile** 接受单个自动局部变量的初始化，指针放在同块该声明之前 | 静态声明、多变量声明、分支/返回/条件表达式、混合捕获、宏、嵌套闭包等继续拒绝；新增多元素协作、静态拒绝及旧 Reshape 声明拒绝测试 |
| guard | 旧规则要求 then 是直接 ReturnStmt；SiLU 使用 CompoundStmt，其中恰有一条 `return failure()`，旧匹配不命中 | **仅 SiLU profile** 接受此块结构，且必须是 `SiLUFusionPattern::matchAndRewrite` 方法体的直属 if | 仍要求内建逻辑或、无 else/init/条件声明；循环内部、其他方法不改；短路次数与幂等性测试；Matmul 六个 guard 回归 |
| 修改计划的协作 | 新 ValueRange 前置数组和捕获指针都插在同一声明前，旧计划会将不同的同位置编辑视为冲突 | 新增显式 `addInsertion`，数组/范围序号 10、捕获指针序号 20；JSON 记录 `insertion_order`，普通编辑为 -1 | 只有不同显式序号的零长度插入可组合；相同序号、插入与替换同起点、其他重叠仍拒绝。协作测试核对相同偏移及 10/20 顺序；旧重叠拒绝测试保留 |

命名空间规则可直接处理 LowerIME 的类外定义和 SiLU 的同空间重开，没有为这两者修改原匹配算法。`if-condition`、`value-range` 是本轮新增规则，未冒充旧规则复用。

这些兼容性说明还写入 pass 配置及各轮 `summary.json.compatibility_notes`。今后发现同规则的新输入不适配，需单独汇报触发结构、旧边界、所做扩展及回归证据；不能只放宽命中数量掩盖遗漏。

## 新规则与接口变化

- `if-condition` 保留左条件优先级：右条件对象只在左条件为假时构造；原 then/else 作用域保留。显式 bool 避免误选否定重载。支持普通 else，保留末尾分号；缩进调整不改多行字符串内容。非平凡析构、引用/volatile、初始化清理、宏源范围和 goto/label 等拒绝。
- `value-range` 使用具名 `mlir::Value[]` → `llvm::ArrayRef<mlir::Value>(pointer, length)` → `mlir::ValueRange(ArrayRef)`。支持多个元素，数组存活覆盖原调用；**所选重载不同于原 initializer_list 构造**。仅允许已审计的局部 Value/EmptyOp 元素，其他显式实参为局部对象，回调为无初始化捕获的直接 lambda。空列表、调用实参/列表元素、有初始化捕获、额外显式参数、嵌套调用和不合适的声明位置会拒绝。

两条新增规则仅启用于 SiLU profile。ValueRange 的 MLIR handle 复制、EmptyOp 隐式转换及非拥有范围仍依赖既有接口审计，不是通用求值顺序或逃逸分析。数组和捕获指针的生命周期不构成外部回调保留引用的安全证明。

## 与独立人工参考的差别

LowerIME 与 072 人工输入的差异仅为空行位置，代码语句一致；未强制逐字节匹配。SiLU 与 104 人工输入在显式 bool、生成名字、完整限定类型、排版和 `(*elementTypePtr)` 括号上有差别，因此只记录字节比较，不要求完全相同。原匹配条件、循环、cast、回调中的算术构建语句、替换/删除操作、pass 运行与注册均保留。

两份 mini 均直接复制所选人工参考的 `mini_mlir.h`，未补写新空实现。边界继续适用：[LowerIME 接口说明](../../LowerIME/test_framaclang/interfaces/README.md)、[SiLU 接口说明](../../TosaElementwiseFusion/test_framaclang/interfaces/README.md)。尤其 SiLU mini 的 function_ref 构造含既有适配占位体，并不执行回调；LowerIME 外部 helper/pattern 实现没有在此转换。

## 完整回归

最终分层实现已重新构建。34 个小程序测试全部通过（原有 20 个 + 本轮 14 个），分成两组 CTest；测试日志（历史自动产物已清理）。新增测试覆盖条件查询次数/优先级/遮蔽、普通 else、多行字面量、短路 guard、两个输入三个输出的范围与捕获协作、名字冲突、幂等性和拒绝路径。

完整 pass + mini 的结果见下表。原生检查列包括：原始 + mini、改写 + mini、改写恢复真实 includes 后的语法/语义检查。Frama-Clang 对照均观测到原始 + mini 退出 4，改写完整输入退出 0 并生成非空 C；对照退出码作为实测结果保留，不作为跨版本假设。

| pass | 原生三组检查 | 重复改写 | Frama-Clang | C 字节数 | 本轮摘要 |
| --- | --- | --- | --- | --- | --- |
| LowerIME | 全部 0 | 0 编辑，字节相同 | 0 | 68954 | summary.json（历史自动产物已清理） |
| SiLUFusion | 全部 0 | 0 编辑，字节相同 | 0 | 165249 | summary.json（历史自动产物已清理） |
| Reshape（旧规则回归） | 全部 0 | 0 编辑，字节相同 | 0 | 722342 | summary.json（历史自动产物已清理） |
| MatmulScalar（旧规则回归） | 全部 0 | 0 编辑，字节相同 | 0 | 121428 | summary.json（历史自动产物已清理） |

Reshape / MatmulScalar 的 `rewritten.cpp`、`baseline_mini.cpp`、`rewritten_real.cpp` 与 2026-11-13 目录回归中的对应产物逐字节相同；MatmulScalar 与独立人工参考的严格字节比较仍通过。新规则接入与目录细分没有改变这两个 pass 的改写源码。

LowerIME：完整 pass（历史自动产物已清理）、mini（历史自动产物已清理）、完整补丁（历史自动产物已清理）、真实依赖检查（历史自动产物已清理）、转换日志（历史自动产物已清理）、生成 C（历史自动产物已清理）。

SiLUFusion：完整 pass（历史自动产物已清理）、mini（历史自动产物已清理）、完整补丁（历史自动产物已清理）、真实依赖检查（历史自动产物已清理）、转换日志（历史自动产物已清理）、生成 C（历史自动产物已清理）。

本轮摘要包含输入/模型/人工参考、当前工具及所有递归实现文件的 SHA-256。成功转换不表示零警告、真实 MLIR 全部实现已转换、完整 pass 已链接执行或一般语义等价证明完成；小样例执行与完整 pass 转换是两种不同证据。
