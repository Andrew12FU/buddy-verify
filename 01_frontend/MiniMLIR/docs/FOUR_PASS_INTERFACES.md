# four_pass 新接口的抽象边界

仅扩展可信 MiniMLIR/include；外部 zip/include 是参考证据，不用于生成派生库。逐文件修改前后 SHA-256、本地真实来源在 interface-abstractions.json 的 extensions。转换副本的每项改写仍在各 pass 的 header-rewrite.json / .patch 中独立记录。

## AssumeTightMemRefLayout

- 范围与容器：补 ArrayRef/MutableArrayRef 的迭代、size/back，SmallVector 的迭代/索引/reserve/迭代器范围构造。保留原 SmallVector 存储和默认内联容量，以及范围构造的输入迭代器约束。
- 迭代器：保留 ValueUseIterator、ilist_iterator、indexed_accessor_range_base::iterator 的类型身份。begin/end、解引用、前置递增与比较是外部声明；后续 ACSL 需要描述迭代位置、有效性、边界与 use-list 的变化。
- 值和区域：补 Value 的 uses/位置/定义操作/父块；BlockArgument、OpResult 保持 Value 基类；Block 与 Region 返回真实的块参数范围和 intrusive iterator。IR 内部存储不构成 ABI 模型。
- 类型与属性：BaseMemRefType/MemRefType 保留 TypeBase、PtrLikeTypeInterface 和 ShapedType trait 路径；布局属性保持 AttributeInterface / AttrBase 路径。参数、返回值、布局句柄及 memorySpace 均按本地接口声明。外部 get/isa/dyn_cast 不赋予固定返回值。
- 操作：ExtractStridedMetadataOp、ReinterpretCastOp、func::ReturnOp 保留本地生成类的完整 Op<> 基类/trait 列表；仅保留当前使用的成员。metadata 返回 base buffer、offset、sizes、strides；ReinterpretCastOp::create 接受 resultType/source/offset/sizes/strides 并返回操作句柄；单结果到 Value 的转换仍来自结果 trait。
- 查询接口：CallOpInterface、BranchOpInterface、RegionBranchOpInterface 保持真实 OpInterface 路径，当前只供类型判别。没有建模调用/控制流算法。
- OpFoldResult 保持 PointerUnion<Attribute, Value>；不增加任意类型转换。OpBuilder 的插入位置操作与 getIndexAttr 保持外部声明，其 IR 副作用待规约描述。
- 原始源码的 enumerate：只建模单范围使用到的声明和 tuple 绑定协议，不复制枚举算法或声称 std::tuple 布局。自动改写只接受 ArrayRef 的两个绑定，并拒绝 index 修改/逃逸；生成代码保存原视图与原长度，不再依赖 enumerate。

## 转换副本中的新兼容操作

1. 按共享 Interface 模板原文生成有界特化，提前完成嵌套 Trait；保留完全限定类型身份。模板正文 SHA 改变则拒绝复用。
2. 仅展开实际触发的 MemRefLayoutAttrInterface 和 OpFoldResult 继承构造；保留原 using 及可用性约束，共享库原件不改。
3. 在 MemRefType 之前显式实例化其原 StorageUserBase，避免生成 C 时出现不完整的基类结构体。
4. walk 的 void 返回包装改为调用后裸 return；非 void 包装保留返回值，以原约束叠加 RetT 的 SFINAE 区分。没有删除遍历或 callback 计算体，也未使用转换器不支持的 if constexpr。

任何转换成功都不是 IR 行为的等价证明；后续 ACSL 仍需约束范围有效性、对象关联、回调时机、IR 访问与修改效果。


## Clang 18 正式补齐：GPU / Depthwise

GPU 的 Option、walk(WalkResult)、GPU 操作、早递增用户迭代器、std::vector 及相关继承/trait 声明已进入同一共享库。early_inc 算法仍在外部：后续 ACSL 必须描述解引用时提前推进、擦除当前用户后的迭代有效性，不能视为普通尾递增循环。

Depthwise 补充 SCF/Vector/Affine/Linalg 操作、AffineExpr 算术及 AffineMap 单结果构造、VectorType、DenseIntElementsAttr 的 int64_t 访问路径、Builder 的零值属性和 I1 类型、ConversionPattern/Target 及 Option 赋值接口。新增操作基类/traits 来自本地生成头文件；DenseTypedElementsAttr 保留 AttrBase、DenseElementsAttr、TypedAttr/ElementsAttr trait 路径。ElementIterator 保留 const 引用返回和 indexed iterator 身份，范围保留 shapeType。外部 create/get/walk 不赋予固定返回值或空效果。

SCF Forall 仍接收 std::optional<ArrayAttr>，没有照搬参考包的 nullopt_t 专用参数。optional 的公开使用边界保持该类型及 nullopt 转换，内部仅用 union/engaged 有界存储；不宣称等于 libstdc++ helper 继承布局。Dense getValues 仅覆盖本次 int64_t 使用，其他 scalar/complex 家族后续另行补齐。function_ref 继续使用既有外部回调抽象，lambda 计算体保留，生命周期/调用时机/效果待 ACSL 规约。

转换副本额外处理 indexed range 具体模板、ConversionPattern 构造，以及 IntegerAttr 触发的 TypedAttr 构造；共享库原件不展开这些继承构造。本轮对比切换前备份的逐文件变化见 CLANG18_HEADER_CHANGES.json，各 pass 的源码/头文件修改分别在 current/<pass>/rewrite-inventory.json 中完整统计。
