# BuiltinTransposeVectorization 接入与 Clang 18 适配

> capture 的 profile 限制和旧拒绝范围为历史记录；当前统一规则见 [GENERIC_CAPTURE.md](GENERIC_CAPTURE.md)。

本轮从桌面 `BuiltinTransposeVectorization.zip` 解压，只读参考保存在
`tests/references/builtin-transpose/`。压缩包 `original.cpp` 与仓库
`TransposeOptimization/BuiltinTransposeVectorization.cpp` 字节一致。
工具固定为 Clang/LibTooling 18.1.8、Frama-Clang 0.0.20（配套 Clang 18.1.8）
和 Frama-C 33.0。不根据压缩包名称推断其原转换器版本。

## 先做头文件对照

1. 原封不动使用压缩包 `rewritten.cpp` 与包内头文件：Clang 18 原生检查通过；
   Frama-Clang 在 `using llvm::StringRef` 的 Using 类型处停止，未生成可回读的 C。
2. 按用户追加要求，保持该 `rewritten.cpp` 字节不变，仅在临时头文件副本中：
   - 将 `using llvm::StringRef` 改为 `using StringRef = llvm::StringRef`。
   - 对 `<cstddef>`、`<cstdint>`、`<initializer_list>`、`<utility>` 使用现有
     `mini/Support/Standard.h` 的有界声明；`std::size_t` 用类型别名声明。
     直接加载实际 libstdc++ 时，转换器会停在 `type_traits` 的依赖名称表达式。
   此对照的原生检查、转换、独立 C 回读均通过。

对照脚本、完整头文件补丁、源码不变检查、生成 C 哈希及命令保存在工作区
`audits/builtin-transpose/`。临时头文件和对照 C 自动清理；正式自动结果只保留一份。
包内 `original.cpp` 直接配包内头文件还缺少传递依赖，原生检查失败；该失败单独记录。
这说明包内模型确有当前转换器的兼容障碍，不能据此证明它曾由配套 Clang 23 的版本转换。

## 历史 origin / rewritten 的区别

除注释、显式依赖 include 和匿名命名空间具名化外，历史改写：

- 去掉七个 `const Value/AffineExpr` 的 const。
- 两个 affine loop 回调由 `[&]` 改为 `[=]`。
- 删除 pattern 的 `affineVectorSize` 数据成员，改为同名文件级全局变量。

最后两项改变对象状态/捕获方式；当前自动版保留原成员和原对象身份。
包内头文件含有返回固定值/空体的 scaffold，本轮仅用作转换兼容对照，
不把这些外部算法实现并入共享 MiniMLIR。

## 正式源码规则及计数

入口：`python3 run.py --pass builtin-transpose`。
每轮使用真实依赖解析 AST；列表规则每轮重新解析，最多 32 轮且要求收敛。
小样例检查不能替代完整 pass 的真实头文件、专用库和转换回归。

| 规则 | 编辑数 | 作用与边界 |
| --- | ---: | --- |
| namespace | 2 | 同一匿名空间的两次重开使用相同具名空间 |
| namespace-reference | 3 | 限定空间外的方法定义、pattern 添加及注册引用 |
| const-local | 7 | 初始化可写句柄存储，原标识符为同对象的 const 引用；初始化一次 |
| init-list | 84 | 列表实参改为具名元素数组及现有 ArrayRef/ValueRange/TypeRange 构造 |
| init-list-preamble | 33 | 在原语句所在块插入数组；不跨条件或循环路径 |
| capture-pointers | 2 | 在两次原 affine 调用前取得原局部对象、引用所指对象及成员地址 |
| capture | 2 | 显式按值捕获上述指针，闭包内按声明身份改用解引用 |

共 133 项文本编辑。列表两轮分别 95/22 项编辑，第三轮为 0；这包含数组前置声明，
并不表示 117 个不同列表。最终重复改写为 0 项编辑。

`BorrowedCapture` 是独立规则模块，通过 capture 分发且限本 profile：
它支持本 pass 的混合类句柄、标量、引用参数和 `this` 标量成员引用。
成员地址指向当前 pattern 的成员存储，不复制 rewriter，不引入全局状态。
未知 this 方法/用法、位域、volatile、重载取地址、混合原值捕获、宏、嵌套或泛型闭包拒绝。
新指针仍依赖原对象生命周期；不声称把逃逸闭包变为安全对象。

列表适配只接纳已审计的不可变 MLIR 元数据构造/查询及既有句柄转换，
不接纳任意副作用函数。原调用路径和列表内元素顺序保留；数组存储持续到原块退出，
不对普通 C++、属性分配次序或任意外部实现声称一般语义等价。
这里只采用本地 LLVM/MLIR 的已知不可变元数据接口；未来 ACSL 仍需定义其效果。

## 旧规则不适配及额外处理

- **capture 的原边界不足**：普通路径仅接受类对象的引用捕获，拒绝引用变量、
  标量混合捕获及 this。增加单独 BorrowedCapture 路径，原 pass 的分支不变。
- **init-list 的原类型/嵌套边界不足**：此前仅支持 Depthwise 的三个类型族，
  会把构造内部的其他列表误当成外层列表，且嵌套列表的语法/语义 AST 共享节点
  产生多个父路径。新增 profile 支持 TypeRange 及 Value/Type/AffineExpr/
  NamedAttribute/Attribute/int64_t/int/bool 的 ArrayRef，使用自身实参的列表、
  所有父路径检查及从内到外的 fresh-AST 轮次；未知调用和条件求值仍拒绝。
- **OneTypedResult 的名字匹配不足**：具体 VectorType 的父类名称包含模板实参。
  新路径按外层模板声明身份识别，未放宽为任意同名 conversion。
- **头文件 indexed-range 白名单需扩展**：新增实际使用的普通 `front()` 成员。
  原结构签名白名单会拒绝；现允许该普通成员被提取/保留，类布局、基类、
  构造和其他成员仍由原有结构哈希约束。
- **Clang 18 的接口 trait 生成顺序**：AffineMapAttr 所需 OpAsmAttrInterface
  触发内部断言，调试定位到 `StorageUserTraitBase<AffineMapAttr, ...Trait>`。
  复用现有同模板完整特化机制，补入 OpAsmAttrInterface、BlobAttr、
  VectorTransferOpInterface、LoopLikeOpInterface 和 vector::MaskableOpInterface。
  特化只进入转换副本，保留类型身份和原模板约束。
- **mapped iterator 的局部闭包存储**：转换器未能将作为字段的局部 lambda 类型
  完整排序，报不完整字段类型。采用已有 function_ref 的外部接口抽象边界：
  不在 MiniMLIR 复制 LLVM 内部 callable 存储/派发实现，保留公开模板类型、
  实参及返回类型、迭代器基类与局部闭包构造适配体。投影 lambda 计算体仍生成 C。
- **共享头文件依赖自包含性**：独立头文件和旧 pass 回归发现新增 getAttr 签名需要 StringAttr 前置声明；已补入 Attributes.h，避免依赖新 pass 的 include 顺序。
- **只有前置声明的枚举**：Clang 接受固定底层类型的 enum 前置声明作为字段，
  本转换器要求枚举定义；按本地生成声明补齐 AtomicRMWKind 的 16 个值。

源码和头文件改写分别在 `rewrite-inventory.json`、`rewrite.json`、
`header-rewrite.json` 及两份 patch 中记录。共享库的接口补充另列于
MiniMLIR 的 `docs/BUILTIN_TRANSPOSE_HEADER_CHANGES.json/.patch`，不混作转换副本编辑。

## 产物与验证范围

正式结果固定为 `../MiniMLIR/current/builtin-transpose/`：
`original.cpp`、`rewritten.cpp`、`converted.c`、`include/`、`conversion-include/`
及改写、提取、工具链和回读记录。仅更新完整通过版本。
两个 affine 回调与一个 LoopReduction 投影闭包的计算体均保留到生成 C。

验证包括完整文件原生检查、真实依赖检查、两份专用库检查、幂等性、
Frama-Clang 转换和独立 C 回读；另复测原有八个完整 pass、规则和头文件检查。
最终实测状态与哈希见 MiniMLIR `current/verification.json`。
转换通过不表示包内 scaffold 与当前外部接口模型行为等价，亦不等于 ACSL 证明完成。
