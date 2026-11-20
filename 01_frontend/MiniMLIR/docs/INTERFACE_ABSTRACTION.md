# 外部接口抽象与后续 ACSL 边界

目标是转换 Buddy pass 自身的计算和控制流，MLIR/LLVM 外部接口的行为后续用
ACSL 规约描述。共享头文件是类型与调用边界的抽象模型，不要求把外部库实现
一起转为 C。当前还没有加入这些行为规约。

## 声明保留规则

1. 保留实际调用所需的限定名字、声明所属类、参数/返回类型、引用限定、默认实参、
   显式性，以及影响重载可用性的模板约束。模板约束用于检查调用绑定，不是行为实现。
2. 保留类型检查需要的基类、别名、traits 和模板结构；不把同名接口改为任意签名。
   实现字段只在抽象模型确实需要时保留，不能由此推断真实 ABI 或完整布局。
3. 外部接口的存储管理、回调调度和 IR 更新算法不因 pass 调用而自动进入转换目标。
   通常只保留声明；后续规约描述其输入条件、状态效果和返回关系。
4. 为模板实例化、类型选择或转换器兼容而必须保留的定义逐项标明。转换适配占位
   与外部算法、行为规约分别记录；空占位体不能作为 `assigns \nothing` 的依据。
5. pass 自己的 lambda/计算体仍是分析目标。抽象 `function_ref` 内部转发，不删除
   lambda 中的 Constant、NegF、Exp、AddF、DivF、MulF 和 Yield 等原始调用语句。

这次修正针对 `function_ref`；共享模型其他已有的短定义未在本轮整体重写。
按声明/成员提取仍只裁剪自有共享模型，不自动决定任意真实库函数的抽象粒度。

## function_ref 的具体处理

合并库此前复用了 Reshape V2 mini 中的 `callback`、`callable`、`callback_fn`
以及构造初始化和调用包装。SiLU 的引用参数包使这段实现触发 Frama-Clang
0.0.20（配套 Clang 18.1.8）的 `params is not a pointer` 注解错误。

当前借鉴历史 SiLU mini 的外部适配方式，保留本地真实 LLVM 的构造模板约束：

- 删除私有函数指针、擦除后的对象地址和 `callback_fn`；不保留转发算法。
- 默认/nullptr 构造、调用运算符、bool 转换和比较只保留外部声明。
- callable 构造保留原签名、两个 enable_if 参数及 lifetimebound 属性；空适配体
  使局部闭包类型的模板构造具有可见定义，标记为 `BUDDY_EXTERNAL_TEMPLATE_ADAPTER`。
- 不承诺类布局、默认构造的平凡性或回调运行已验证。原 Ret=void 转发包装的限制
  不再存在；签名检查同时覆盖 void/int 返回值、错参拒绝和返回值不兼容拒绝。

未来 ACSL 接入需描述视图与 callable 的关联、非拥有生命周期以及调用效果；
必须同时处理构造适配体，不能把空体按无状态变化进行证明。具体逻辑状态和回调
契约形式留待规约阶段决定，本轮没有添加虚构的固定返回值或空行为契约。
`GenericOp::create` 本来就是外部声明；它构造 region/调用 lambda 的效果也需规约，
生成 C 中存在 lambda 体并不表示框架已经执行或验证了该回调。

## 追溯层次

`docs/interface-abstractions.json` 是共享模型抽象调整的审查记录，保存本次修改
完整前后文本、字节长度、SHA-256、本地真实签名和历史适配来源。
它与每轮 `header-rewrite.json` 的转换兼容改写独立。

每次提取检查记录与当前共享头文件的哈希是否一致，随后保存
`interface-abstractions.json` 快照；manifest 和 `rewrite-inventory.json`
引用该快照。快照中的 `headers_in_include_closure` 仅表示头文件属于 include
闭包，不表示该类一定被当前 pass 使用。一次共享模型调整不能累加成四次 pass
兼容编辑。运行结果只保留 current 的通过版本；历史人工 mini 不修改。
