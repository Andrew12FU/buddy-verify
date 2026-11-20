# four_pass：原始/参考改写审计与接入

当前以 four_pass(1).zip 的 Clang 18 参考为主；两包八份 C++ 文件逐字节相同。本文件中的源码差异审计同时适用于两包。正式八个 pass 均已接入，版本、结果与新增规则见 [CLANG18_RETURN.md](CLANG18_RETURN.md)。

原包完整解压在 `tests/references/four_pass/`。四份 original.cpp 与仓库真实 pass 源码逐字节相同；每个目录的 comparison.patch 记录完整差异，archive-manifest.json 记录来源哈希。原包 include 是历史参考，不替换可信 MiniMLIR/include。

| 顺序 / pass | original → rewritten 中实际出现的改写 | 现有规则关系 |
| --- | --- | --- |
| 1 AssumeTightMemRefLayout | 匿名空间具名和注册类型限定；8 个 range-for（含 2 个 enumerate 结构化绑定）展开；worklist 引用捕获改指针 | namespace/capture 可复用；新增 range-for/enumerate |
| 2 EliminateMemRefCopy | 匿名空间；6 个 if 条件声明外提；5 个 range-for（含 2 个 enumerate）；继承构造改显式构造；一个 `||` 拆两个 if；末维 stride 分支改 else-if | if-condition 原规则只接收复合 then；guard 原规则只允许原两个 profile；构造和 range 新增 |
| 3 ConvertMemcpyToGPU | 匿名空间；9 个 if 条件声明外提；3 个 range-for；两个 lambda 的捕获改写（一个删除未使用的默认捕获）；SmallVector 范围构造改逐项 push_back；若干 Op 的 `.getResult/getOperand/getNumOperands` 改 `->` | 部分由包内 mini 简化导致，不能据此改真实 pass；else-if 与 early_inc 需修正，见下 |
| 4 DepthwiseConvNhwcHwc | 两次匿名空间声明和外部限定；7 层 lambda 环境改结构体值捕获并逐级复制；ValueRange 列表改 0/1/2/3/4 元构造；SmallVector 列表改 push_back；6 个 const 对象初始化拆临时；Op `.getResult` 改 `->` | 嵌套捕获可复用逐层指针，不照搬环境对象复制；列表规则原仅 SiLU 单调用形状，需扩展 |

## 不可直接照搬的参考差异

1. GPU `llvm::make_early_inc_range(result.getUsers())` 被替换成普通 user_range 的尾递增遍历，循环内还会 erase user。这丢失提前推进迭代器的要求。自动规则必须保留 early_inc 适配器及其递增协议。
2. GPU 的 alloc/copy/global/return 及 memory-space 分支转换被一起提前执行，改变原 else-if 的惰性求值和条件对象生命周期。自动规则只在到达对应分支时构造，在同一原 if 作用域内处理。
3. Depthwise 将引用捕获改为环境结构体的值复制，不是通用引用捕获等价变换。自动规则沿用原对象指针和逐层局部指针别名，保留原 lambda 计算体。
4. 包内 ValueRange 多元构造、弱化 Op 继承/traits、function_ref 固定元数等接口偏差只作为历史证据；真实原生依赖检查优先，任何新外部接口抽象必须单独记录来源。
5. Eliminate 的 `||` 使用现有块内 bool 方案，不机械拆分；stride 分支重排没有转换必要时保留原代码。

## 接入记录

按上述顺序逐项接入；只有完整 pass + 派生 MiniMLIR + Frama-Clang 0.0.20（Clang 18.1.8）转换 + 生成 C 回读全部通过，才进入 MiniMLIR/current。失败诊断使用临时目录，避免历史结果堆积。新规则及原规则边界不适配在本文件补充。

### 已完成：AssumeTightMemRefLayout

固定产物：MiniMLIR/current/assume-tight。源改写 8 项（其中 3 个 range-for 树包含 8 个循环），namespace/capture/if-condition 复用，新增范围循环与两个 enumerate 绑定展开。原始/改写 mini、真实依赖、Clang 18 检查、重复改写、Frama-Clang 转换、C 回读均通过。共享接口扩展见 MiniMLIR/docs/FOUR_PASS_INTERFACES.md。

额外不适配：接口提取原先漏范围循环隐含方法、模板偏特化成员、结构化绑定的 tuple_size/tuple_element/get 依赖；现按 AST 隐含调用及编译器 lookup 补齐，新增回归。

### 已完成：EliminateMemRefCopy

新增 constructor：在保留 using 的前提下显式转发当前使用的 MLIRContext* 构造；没有按 zip 删除其余继承重载。guard 沿用块内 bool；原 stride 的 continue 分支保留。


### 已完成：ConvertMemcpyToGPU / DepthwiseConvNhwcHwc

GPU 保留 early_inc 和 else-if 惰性求值，使用真实 inherited result/operand 查询及 SmallVector 范围构造。Depthwise 沿用逐层引用捕获指针，并新增 const-local、init-list 两条规则，保留 lambda 计算体。正式产物分别位于 MiniMLIR/current/gpu-memcpy、depthwise-conv。完整回归与不适配记录见 CLANG18_RETURN.md。
