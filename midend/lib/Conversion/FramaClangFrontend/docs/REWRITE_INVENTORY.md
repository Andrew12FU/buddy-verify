# 源码与头文件改写清单

工具版本分别记录：前端解析/提取使用 Clang/LibTooling **23.1.6**；转换器为
**Frama-Clang 0.0.20**，配套 LLVM/Clang **18.1.8**；C 回读使用 Frama-C 33.0。
这里的 18.1.8 是 Clang 版本，不是 Frama-Clang 的发行版本。

本清单覆盖 Reshape、MatmulScalar、LowerIME、SiLUFusion 的当前默认共享库流程。
每轮自动生成 `rewrite-inventory.json`：分别记录源码和头文件规则、命中数、全部
字节编辑、前后文字及原报告哈希。默认只保留 current 的通过版本；显式诊断运行才保留失败改写。
“改写已应用”不代表“转换已通过”。

## pass 源码规则

| 能力 | JSON 规则名 | 作用与当前边界 |
| --- | --- | --- |
| 匿名命名空间 | namespace、namespace-reference | 具名化并限定外部类型引用；改变内部链接属性，仅处理单翻译单元转换副本 |
| while 条件声明 | while | 每轮显式声明和 bool 判断，保留循环体作用域；有非平凡清理、跳转等情况拒绝 |
| 对象引用捕获 | capture、capture-pointers | 改为对象指针按值捕获；嵌套组逐层复制指针，按声明身份替换，不按变量字符串猜测 |
| failure guard | guard | 对各 profile 已审计的直属 if 增加作用域块；Matmul 在块内用新 bool 保存原条件以保留生成 C 的作用域；不拆分短路表达式、不移动外部变量 |
| if 条件声明 | if-condition | SiLU 的条件声明/else-if 展开，保留右分支初始化时机和作用域 |
| ValueRange 列表 | value-range、value-range-preamble | SiLU 已审计实参改为具名数组、ArrayRef 和 ValueRange，保持数组生命周期 |

详细适用/拒绝条件及实现入口见 [README](../README.md) 与
[ARCHITECTURE](../ARCHITECTURE.md)。嵌套 lambda 属于捕获规则内部规划器，
不是另一个可独立重复执行的 CLI pass。

| 能力/编辑数 | Reshape | MatmulScalar | LowerIME | SiLUFusion |
| --- | ---: | ---: | ---: | ---: |
| namespace + namespace-reference | 2 | 2 | 3 | 3 |
| while | 1 | 0 | 0 | 0 |
| capture + capture-pointers | 4 | 0 | 0 | 2 |
| guard | 1 | 6 | 0 | 1 |
| if-condition | 0 | 0 | 0 | 1 |
| value-range + value-range-preamble | 0 | 0 | 0 | 3 |
| 合计 | **8** | **8** | **3** | **10** |

当前四个完整 pass 共 **29 条源码字节编辑**。原始 Buddy 文件不修改，产物为
`rewritten.cpp`。默认共享库流程保留原 include 路径，因此没有 include 替换编辑。
历史显式 `--interface` 模式的 `interface-substitution` 仍保留，不混入本表。

## 头文件规则

共享模型的外部接口抽象先于下列转换兼容规则。`function_ref` 现移除内部存储/
转发实现，保留签名和模板约束，后续行为由 ACSL 描述。该项是 **1 次共享模型
抽象调整**，不计入下表的 4 条转换副本编辑；完整前后文本、来源、适配占位边界
见 [interface-abstractions.json](../../MiniMLIR/docs/interface-abstractions.json) 与
[INTERFACE_ABSTRACTION.md](../../MiniMLIR/docs/INTERFACE_ABSTRACTION.md)。

唯一维护的接口模型是 `MiniMLIR/include`。AST 成员提取得到每轮 `include/` 后，
按顺序执行下列规则，写入独立 `conversion-include/`。两份生成库均不手工修补。

| JSON 规则名 | 作用 | 适用对象/约束 |
| --- | --- | --- |
| typed-attr-interface-specialization | 从原 Interface 模板逐 token 代入实参，生成同名完整特化并显式实例化对应 AttributeInterface | 所选库含 TypedAttr 完整定义；保留限定类型身份、基类、字段、别名、参数及模板约束 |
| typed-attr-constructor-expansion | 增加带原 trait 约束的 TypedAttr 构造声明，并恢复其默认构造 | 所选库含完整 FloatAttr；原继承构造在共享库和提取库中保留，仅转换副本增加声明 |
| op-nullptr-constructor-expansion | 从 Op(nullptr) 原型生成 SigmoidOp(nullptr) 声明，并恢复默认构造 | 所选库含完整 SigmoidOp；保持 nullptr 隐式构造、Operation* 显式构造等可用性 |

构造规则经用户明确同意后启用。这里展开的是**接口声明**，没有补写外部构造算法，
不能据此声称构造行为或完整 MLIR ABI 已建模。模板及类形状失配会拒绝输出，
不能静默放宽约束。规则、来源与代码层次见
[HEADER_REWRITES.md](../../MiniMLIR/docs/HEADER_REWRITES.md)。

| 头文件规则/编辑数 | Reshape | MatmulScalar | LowerIME | SiLUFusion |
| --- | ---: | ---: | ---: | ---: |
| Interface 完整特化 | 0 | 1 | 0 | 1 |
| TypedAttr 构造声明 | 0 | 0 | 0 | 1 |
| SigmoidOp(nullptr) 构造声明 | 0 | 0 | 0 | 1 |
| 合计 | **0** | **1** | **0** | **3** |

共 **4 条头文件字节编辑**，涉及两个不同的头文件路径：`BuiltinAttributes.h` 和
`TosaOps.h`。一条编辑可以插入多个声明；编辑数不是 API 数、头文件数或提取声明数。
成员提取的删除范围另见 `interface-manifest.json`，不混算为兼容改写。

## 坐标与后续自动提取入口

- `rewrite.json` 的所有源码编辑都以原始 pass 的 UTF-8 字节坐标为基准。
- `header-rewrite.json` 的头文件编辑按 `sequence` 顺序执行，坐标相对于此前编辑
  已经应用的当前头文件。顺序重放可以精确复现生成结果；不要逆序当作同一原文编辑。
- `rewrite-inventory.json` 汇总上述两类完整编辑，保存原报告/产物哈希，并在
  `extraction_inputs` 中引用改写后的 pass、声明依赖计划、共享库→专用库 manifest
  和专用库→转换库报告。

下一步自动提取应以改写后的 pass 分析依赖，以共享模型/真实库来源界定接口，
将转换兼容改写作为独立派生层。不得将构造兼容声明反向写成共享库中的原始接口，
也不能把现有“自有模型上的成员提取”当作已经实现“任意真实 MLIR 头文件抽象”。

## 额外不适配记录

1. 历史 capture 规则不支持变量初始化中的 lambda、guard 不支持单条 return 的
   then 块；SiLU profile 中已有限扩展。同位置前置声明采用显式顺序组合，详见
   [IME_SILU.md](IME_SILU.md)。
2. 仅调整模板声明顺序不能完整解决 TypedAttr 问题：会进一步出现嵌套 Trait
   定义缺失。因此使用保留同名类型的完整特化展开，没有采用移走 Trait 改变类型名
   的方案。
3. 仅展开 TypedAttr 构造还不能覆盖 SigmoidOp(nullptr) 的同类继承构造；新增了
   单独的 nullptr 规则，默认构造因新增构造声明而受抑制的问题也一并处理。
4. SiLU 曾暴露 function_ref 回调参数包的引用参数注解错误。根因是共享模型
   引入了应当抽象的外部转发实现。当前修正外部接口边界，保留签名/约束和 pass
   自己的 lambda 体；没有继续展开转发算法，也没有关闭注解检查。见上述共享模型
   调整及最新回归记录；其构造空适配体不是无副作用的行为规约。
5. Matmul 此前生成 C 回读报 goto 跨越 lhsTy 初始化；旧单头 mini 生成物也复现。
   原规则仅包住 if：纯标量条件不产生局部临时声明，转换输出会展开这个块，使
   短路 goto 与带析构标记、声明后赋值的 lhsTy 处于同一块。现改为块内新增具名
   bool 保存完整原条件，再 if-return。名字避开全部已有标识符；保留 || 的求值
   次数/顺序、原 return 和外部变量生命周期，不拆分条件，也不移除析构标记。
   六个 guard 的替换文字更新，编辑数/范围不变。历史人工参考保留，新的严格
   源码参考为 `tests/references/matmul-scalar-guard-binding.cpp`，没有关闭参考匹配。

完整回归状态与失败日志见 [VALIDATION.md](../../MiniMLIR/docs/VALIDATION.md)。

## BuiltinTransposeVectorization

新增完整 pass 的 133 项源码编辑、头文件兼容扩展及旧规则不适配见 [专门记录](BUILTIN_TRANSPOSE.md)。源码与头文件仍在 current 的两份清单中分别统计；共享接口补充单独记录于 MiniMLIR/docs/BUILTIN_TRANSPOSE_HEADER_CHANGES.json。
