# 来源与统一边界

结构参照桌面的 `four_pass(2).zip`：共享 include 树、原头文件引用路径。
没有复制包内省略 SFINAE 的 ArrayRef、全局 Op→Value 转换或固定值回调等简化。

## 唯一公共定义的来源

- Reshape V2 已审计 mini 提供公共 C++ 标准声明适配、LLVM 支持层、MLIR
  类型/值/操作、Pattern、Pass 等较完整的结构基础。拆分时保留原声明文字，
  每个迁移声明旁记录原行号；完整迁移索引在 `bootstrap-inventory.json`。
- Matmul、IME、SiLU 历史 mini 提供新增的已使用接口清单；与本地 LLVM/MLIR
  源码及生成 `.inc` 核对参数、返回值和成员所有者后合入公共定义。
- `sources.json` 保存这些只读来源和本地仓库版本的哈希。历史 mini 仍保留，
  但不是生成专用库的输入。

## 合并时修正的模型差异

- 同名 Value、LogicalResult、Type、Pass 等不再按 pass 定义为空类、带状态类等
  不同版本；全部从共享库引用同一个抽象定义。
- ArrayRef 使用真实的 data/size 可用性及类型约束，不使用无约束容器构造。
- 保留 SmallVector 的存储与默认 N 计算，不按 pass 手写 N 的替代表。
- `RankedTensorType::getRank` 保留 trait 所有者及 using；`getShape/getElementType`
  保留本地生成类上的声明，不将全部接口平铺为任意类成员。
- 单结果 op 的隐式转换放在结果 trait 上；`TypedValue<Type>` 遵循真实别名选择
  为 Value，不再沿用旧 SiLU 中多余的 `detail::TypedValue<Type>` 结果层。
- TypedAttr/ElementsAttr 保留 AttributeInterface 路径；FloatAttr/IntegerAttr
  保留 AttrBase 与 TypedAttr trait。没有为绕过 Frama-Clang 而替换继承构造。
- function_ref 合并时曾复用 Reshape V2 的存储和转发包装；现按外部接口抽象边界
  移除这部分实现，借鉴旧 SiLU 的局部 lambda 构造适配，但保留真实构造模板约束。
  修改前后文字和来源见 `interface-abstractions.json`，规则见
  [INTERFACE_ABSTRACTION.md](INTERFACE_ABSTRACTION.md)。外部行为待 ACSL 描述。

## 仍然有界的部分

这里不是原库的 ABI/完整布局或算法实现：Reshape 原审计中未建模的存储算法、
std 容器实现、部分外部成员仍保持原有边界。新增的 concrete op 声明保留实际
使用的 Op/OpState 与结果 trait 路径，尚未补齐它们所有未使用的 ODS mixin。
IntegerType 未使用的接口 mixin 同样未补齐。不得据此对任意新类型查询或新重载
作出等价承诺；新增用途必须扩充并审计共享模型。

为让转换器看见模板基类定义，模型包含部分显式实例化。这没有引入替代类型。
这些实例化属于共享库结构基础，不能由单个 pass 私自删除或替换。

## 用户决定保留的转换障碍

SiLU 的 `FloatAttr → TypedAttr` 隐式转换按真实路径使用继承构造。
23 版 Frama-Clang 历史实验报 `Unsupported expression: CXXInheritedCtorInitExpr`。
用户起初选择保持该构造形式，先报告受阻。随后明确允许仅在转换用副本中展开
带原约束的构造声明；共享原库继续保留真实形式，差异记录在 header-rewrite.json。

头文件兼容改写前，Matmul 和 SiLU 在共享 `BuiltinAttributes.h` 的
模板接口/trait 结构中更早触发 `VisitTable.h:217 removeWaitWithinClass` 断言。
原生 18/23 均接受这些声明。当前通过 [独立头文件改写](HEADER_REWRITES.md)
在转换副本中展开原模板的完整特化，已经消除该断言；共享原库本身没有变动。
SiLU 随后在配套 Clang 18.1.8 的 Frama-Clang 0.0.20 中遇到继承构造问题，
现已在转换副本中处理 TypedAttr 和 SigmoidOp(nullptr)。随后暴露的回调注解错误
来自不必要地导入外部转发实现；当前已修正共享模型的抽象边界，见上述抽象记录。
Matmul 的历史回读问题通过 guard 块内具名 bool 保存条件结果处理，不再仅包 if。
此项属于 pass 源码兼容改写，不改变共享接口模型。详见 [VALIDATION.md](VALIDATION.md)。
