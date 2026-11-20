# Clang 18 正式前端恢复（2026-11-18）

## 当前正式工具链

- 解析、AST 改写、声明/成员提取：Clang/LibTooling **18.1.8**，构建目录 `build-llvm18/`。
- C++→C：**Frama-Clang 0.0.20，配套 Clang 18.1.8**，由 Frama-C 33.0 驱动。
- C 回读：Frama-C 33.0。普通 clang++ 的语法检查不代替 Frama-Clang 转换。
- `build_frontend.py`、CMake、`run.py` 的默认配置均为 18。23 只保留为历史环境及显式实验配置。

## 切换前备份

工作区 `backups/frontend23-before-llvm18-20261118-051817/` 内：

- `frontend23-snapshot.tar.gz`：切换前完整前端、共享 MiniMLIR、六份 current 结果和相关 23 实验材料。
- `manifest.json`：2290 个文件的 SHA-256；`verification.json`：归档逐文件回读核验及六个成功记录的工具链。
- 六份结果为 Reshape、MatmulScalar、LowerIME、SiLUFusion、AssumeTight、EliminateCopy，均保留 original.cpp、rewritten.cpp、converted.c 和两份派生头文件树。

这六份历史成功记录使用的是 **23 前端 + 18 转换器**。保存时的最新开发树还包含未完成的 GPU/Depthwise 扩展；备份不宣称该开发树未经修复即可重新生成六份历史结果。断电前的重试确实发现旧模型遗漏 AffineMap::get、friend 依赖闭包等问题，已经单独保留审计结论。

## four_pass(1) 与 four_pass(2) 的比较

对桌面两个 zip 的同名文件逐字节比较：四份 original.cpp、四份 rewritten.cpp 全部一致，且四份 original.cpp 与仓库源文件一致。因此“pass 改写一致、区别在头文件”这一判断成立；此外头文件偏差说明文档也有变化。

11 个同路径代码头文件内容不同；`FORCED_DEVIATIONS.md` 不同；(1) 独有 `mlir/Dialect/Linalg/IR/LinalgOps.h`，(2) 独有 `llvm/ADT/iterator_range.h`。主要实质差异包括 SmallVector 构造的 explicit、GPU builder 的 Type/TypeRange 参数、metadata getter 返回类型、PatternBenefit 存储宽度、Rewriter/Pass 的继承和虚接口、ConversionTarget 的返回类型与 ConversionPattern 的重载。并非仅修改 include 路径或注释。

(2) 的 GPU 人工代码在本地 Clang18 语法检查中失败：MemcpyOp/DeallocOp 的新头文件接收 Type，而 unchanged rewritten.cpp 仍传 TypeRange。这是包内源码/接口的组合不一致；当前正式共享库按真实调用的重载修正，未把 TypeRange 随意改成 Type。

(1) 的四份人工 rewritten.cpp 已在本地 **Clang 18 + Frama-Clang 0.0.20（18.1.8）** 完成语法、转换和 C 回读检查。包内 release/21.x、release/23.x 字样描述的是接口参考分支，不能独立证明原运行使用哪个 Clang。运行版本以当前工具输出和日志为准。

原始快照分别保留于 `tests/references/four_pass_clang18/`（首要参考，来自 1）和 `tests/references/four_pass/`（来自 2）。机器可读差异见工作区 `audits/frontend18-return-20261118-051817/archive-differences.json`。

## 正式接入及产物

唯一可信接口模型仍是 `../MiniMLIR/include/`。每个 pass 从它提取声明/成员，再生成转换兼容副本；不会直接将 zip/include 作为正式运行库。

| 参数 | 自动源编辑数 | 状态 |
| --- | ---: | --- |
| reshape | 8 | 转换及 C 回读通过 |
| matmul-scalar | 8 | 转换及 C 回读通过 |
| lower-ime | 3 | 转换及 C 回读通过 |
| silu-fusion | 10 | 转换及 C 回读通过 |
| assume-tight | 8 | 转换及 C 回读通过 |
| eliminate-copy | 15 | 转换及 C 回读通过 |
| gpu-memcpy | 11 | 转换及 C 回读通过 |
| depthwise-conv | 60 | 转换及 C 回读通过 |

计数是文本编辑数，不是循环/lambda 的个数；一项可以覆盖一整棵嵌套树。Depthwise 的 60 项包含 24 个列表替换及 23 个数组前置声明。

每个固定目录 `../MiniMLIR/current/<参数>/` 保存 `original.cpp`、`rewritten.cpp`、`converted.c`、`include/`、`conversion-include/`、源码/头文件补丁、逐项规则清单和 summary.json。八个 pass 均检查原始/改写专用库、真实依赖、转换副本、重复改写及 C 回读；原始未改写代码的转换是失败对照，不要求通过。

## 新规则及旧规则不适配

1. **Clang 18 AST 差异**：默认遍历不包含写出的显式模板实例化；提取器显式登记其源范围和依赖。偏特化被保留时同时保留主模板；friend wrapper 同时保留其函数。没有据此改变 Buddy pass 算法。
2. **嵌套类迁移旧规则**：原规则只接受完整 UseIterator 类体，而提取器会删除未使用普通成员。现允许已审计成员的合法子集，固定基类、字段、构造及类型身份仍严格检查；不恢复多余成员。
3. **TypedAttr 旧规则**：过去只根据 FloatAttr 定义启用，漏掉 Depthwise 的 IntegerAttr 输入。现复用同一原始 trait 约束，在转换副本中声明对应构造；共享库仍保留继承构造。
4. **indexed range 转换规则**：Frama-Clang 对通用模板内嵌套 iterator 的生成顺序会留下不完整类型。转换副本从选中的模板原文生成 ValueRange/TypeRange/ResultRange 的具体特化，保留身份、字段、基类、参数和当前选中成员。
5. **ConversionPattern 继承构造**：转换副本显式声明当前 rootName/benefit/context/generatedNames 构造，保留 using 和默认参数，外部构造行为继续抽象。
6. **const-local（新增源码规则）**：Depthwise 的六处 const Value/AffineExpr 初始化改为可写存储＋同名 const 引用，仍选中只读重载；不把 const 句柄直接改成可写变量。当前规则仅用于已审计 pass，不宣称覆盖任意 const_cast/decltype 程序。
7. **init-list（新增源码规则）**：Depthwise 的 ValueRange、SmallVector<OpFoldResult,3>、ArrayRef<int64_t> 列表使用独立名字的数组和已有真实范围构造；按原 block 保持元素生命周期，避免转换器复用 init_array/init_list 名称。条件求值上下文、宏或不在已审计查询族的副作用被拒绝。

仍不照搬 zip 中 GPU 的普通尾递增 user 遍历、else-if 查询全部提前执行、Depthwise 的环境对象逐层值复制或 ValueRange 假多元构造。理由见 FOUR_PASS_IMPORT.md。

共享头文件本轮变化、真实来源及前后哈希在 `../MiniMLIR/docs/CLANG18_HEADER_CHANGES.json`；每次正式运行还保存模型审计快照及 header-rewrite.json。外部接口行为尚未接入 ACSL；optional 等标准库内部存储采用有界模型，不是 ABI 模型，也不是实际 MLIR 运行实现。
