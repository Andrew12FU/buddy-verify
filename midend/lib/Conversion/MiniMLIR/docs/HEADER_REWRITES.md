# 转换用头文件改写

共享 `MiniMLIR/include` 继续保存唯一的接口模型。`extract.py` 按 AST 闭包生成
`include/` 专用库，新增 `rewrite_headers.py` 再从专用库生成独立的
`conversion-include/`。两份生成库均不手工维护，兼容规则不修改 Buddy pass 或
共享原库。共享模型自身的外部接口抽象另见
[INTERFACE_ABSTRACTION.md](INTERFACE_ABSTRACTION.md)：function_ref 的外部实现
移除属于模型抽象调整，不属于下面三条转换兼容规则。

```text
共享模型 → AST 声明/成员提取 → include/
                               ↓ 兼容规则
                         conversion-include/ → Frama-Clang 0.0.20（Clang 18.1.8）
```

## 当前规则：typed-attr-interface-specialization

适用范围是已提取库保留了 TypedAttr 完整定义的情况，目前为 Matmul、SiLU。
Reshape、IME 不需要该完整定义，规则输出零条编辑。

从专用库的 `InterfaceSupport.h` 读取 `detail::Interface` 模板定义，将以下
形参替换为实际模板实参，生成同名类的显式完整特化，并插入到 TypedAttr 定义前：

| 形参 | 实参 |
| --- | --- |
| ConcreteType | `mlir::TypedAttr` |
| ValueT、BaseType | `mlir::Attribute` |
| Traits | `mlir::detail::TypedAttrInterfaceTraits` |
| BaseTrait | `mlir::AttributeTrait::TraitBase` |

随后显式实例化对应的 `AttributeInterface` 基类，使 Frama-Clang 按定义顺序
生成所需类型。仅补充显式实例化而不展开完整特化，会从原来的断言变成生成中间
表示缺少 `Interface::Trait` 类定义；不能将这种结果视为成功。

特化体直接由原模板替换形参得到，没有另一套手写字段/构造定义。类及嵌套 Trait
的限定类型名、基类、字段、成员别名、构造参数、默认值和模板约束保持对应关系。
它改变的是该模板组合的实例化表达形式。不会把 Trait 移为另一个类、平铺继承，
也不放宽 `is_base_of` 约束。共享原库仍保留继承构造；转换副本中的后续构造规则
见下节。

这是针对自有有界模型的规则，不是通用 C++ 实例化器：参数签名和模板体哈希必须
与已审计形状一致。替换只作用于标识符 token，注释和字符串不替换。模板发生变化
会拒绝输出，必须先审计规则；不能只更新期望编辑数来绕过检查。

## 经用户允许的构造声明展开

`typed-attr-constructor-expansion` 从原 Interface 构造模板取得原型，在转换副本的
TypedAttr 中增加对应声明。原来的 `Trait<T>` 查找显式绑定到
`InterfaceBase::Trait<T>`，保持原基类约束，而不是绑定到派生类的另一层 Trait。
现有继承重载保留；新增构造会抑制隐式默认构造，因此同时添加 defaulted 默认构造。

`op-nullptr-constructor-expansion` 根据原 `Op(std::nullptr_t)` 原型，为 SigmoidOp
增加同样的 nullptr 构造声明并恢复默认构造。它解决的是 TypedAttr 之后新暴露的
同类继承构造问题；不将 Operation* 构造变成隐式，也不改派生关系。

这些是转换模型中的**声明展开**，不是外部构造算法的实现。每项记录原型来源和
抽象影响；canonical 共享库及原提取库保持原样。只在完整 FloatAttr/SigmoidOp
被当前 pass 选中时执行，因此 Matmul 不会额外得到 SiLU 的构造适配。

## 文件层次和参数

- `rewrite_headers.py`：CLI，读取路径并调用流程层。
- `python/rewrites/pipeline.py`：来源哈希、输出保护、规则顺序和产物。
- `python/rewrites/typed_attr.py`：唯一的 TypedAttr 规则及形状前提。
- `python/rewrites/constructors.py`：两种已审计构造的声明展开、默认构造保护。
- `FramaClangFrontend/experiment/validation.py`：在成员提取和原生接口检查后运行
  此阶段，再分别检查兼容副本，最终转换只使用兼容副本的模型搜索路径。

```sh
python3 rewrite_headers.py --source /run/include \
  --manifest /run/interface-manifest.json --output /run/conversion-include
```

`--source` 和 `--manifest` 为只读输入；`--output` 必须是新的目录，不得位于
共享库或输入库内。输出旁已有改写报告时也拒绝覆盖。所有规则先在内存中完成，
来源/规则不匹配时不创建结果目录。

`header-rewrite.json` 记录输入/输出头文件哈希、模板来源、实际参数和 UTF-8
字节编辑；`header-rewrite.patch` 提供可直接审查的差异。编辑按 `sequence` 顺序
重放，每项坐标相对于此前编辑已经应用的当前头文件，不能逆序当作同一原文编辑。
原 `interface-manifest.json` 仍只描述未改写的提取库。
跨阶段的完整规则统计见 [REWRITE_INVENTORY.md](../../FramaClangFrontend/docs/REWRITE_INVENTORY.md)。

## 回归范围

针对用例对原库和兼容副本分别检查 Trait 的基类/成员类型身份、模型内布局、
FloatAttr→TypedAttr 可构造/可隐式转换，以及无关类型被约束拒绝、Attribute
构造仍为 explicit；也检查两类默认构造、nullptr 构造和 Operation* 的显式性。
另验证字节编辑重放、幂等性、模板/原型变化拒绝、原件哈希与历史输出保护。
四个完整 pass 继续执行真实依赖、原始专用库、兼容库的原生检查及 Frama-Clang
转换和 C 回读，结果见 [VALIDATION.md](VALIDATION.md)。这些检查不证明完整 MLIR ABI
或外部构造算法的语义等价。

BuiltinTransposeVectorization 在转换副本增加五种既有 Interface 模板的完整特化，并扩展 indexed range 普通成员 front 的审计白名单；共享接口和 callable 抽象边界见 [记录](BUILTIN_TRANSPOSE.md)。
