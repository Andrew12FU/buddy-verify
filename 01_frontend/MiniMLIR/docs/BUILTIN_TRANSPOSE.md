# BuiltinTransposeVectorization 的共享接口扩充

该 pass 采用唯一维护的共享库，再按声明/成员提取专用库，最后生成独立的
Clang 18 转换副本。参考压缩包不替代共享库，也不沿用其中外部算法的固定返回值实现。
源码及转换副本规则见相邻 FramaClangFrontend 的
[接入记录](../../FramaClangFrontend/docs/BUILTIN_TRANSPOSE.md)。

## 头文件接口目录

| 接口族 | 本轮保留的调用/类型 |
| --- | --- |
| Affine | AffineIfOp、AffineParallelOp、AffineYieldOp 的真实 Op 基类/trait；IntegerSet、AffineExpr 减法、buildAffineLoopNest 的两个重载 |
| Vector | TransferReadOp、TransferWriteOp 的真实 Op 基类/trait，使用 TypeRange/ValueRange/NamedAttribute 的 create 声明 |
| Linalg | TransposeOp 的真实 Op 基类/trait及 getOperationName |
| 属性/Builder | StringAttr、NamedAttribute、AffineMapAttr、DenseArrayAttr/Impl 的类型结构与别名；本 pass 的元数据查询声明 |
| 容器/视图 | ValueRange::getTypes 的 ValueTypeRange/ValueTypeIterator 类型身份；ArrayRef/SmallVector 使用既有构造，indexed range 新增 front 声明 |
| IR 支持 | Block 构造/析构/addArgument、Region::push_back、Operation::getAttr(StringAttr)、OneRegion::getRegion |
| 分析接口 | LoopReduction 的三个实际字段，AtomicRMWKind 的完整枚举值 |
| 标准声明 | int32_t、remove_const_t 和 cstdint 有界入口，不加载 libstdc++ 实现 |

每个声明的形参、返回类型和基类以本地真实 LLVM/MLIR 为准：
`llvm/mlir/include/mlir/{IR,Dialect/Affine, ...}` 以及构建目录
`build-v0.0.4/llvm/tools/mlir/include/mlir/` 的生成声明；LLVM 容器以
`llvm/llvm/include/llvm/ADT/{STLExtras,SmallVector}.h` 为准。
声明正文不实现外部 IR 构建、属性查找、循环构建或内存算法。
22 个改动文件的前后哈希和完整差异见
[机器清单](BUILTIN_TRANSPOSE_HEADER_CHANGES.json) 与
[完整补丁](BUILTIN_TRANSPOSE_HEADER_CHANGES.patch)。

## 外部 callable 抽象

本 pass 的 `to_vector<4>(map_range(ArrayRef<LoopReduction>{}, lambda))` 引入
局部闭包作为 mapped iterator 的模板实参。保留 mapped_iterator 类型身份、
迭代器基类、投影结果类型及构造参数；局部闭包构造有可见适配体。
map_range/to_vector 的有界包装体仍创建相应视图/SmallVector，未改成固定空返回。
本轮使用范围是具有 begin/end 的 ArrayRef 和普通可调用投影，不声明支持任意 ADL
容器或成员指针 callable。

LLVM 内部 `callable_detail::Callable` 的 optional 存储、派发与其实现不在抽象库中
复制；对应 callable 关联、有效性和调用效果属于未来外部契约。
这与已有 function_ref 的边界一致：空适配体不意味着无副作用，也不意味着忽略 callback。
公开调用签名的存在与计算体保留，尚不足以证明真实外部算法如何调用它。
不声称内部存储布局或 ABI 与 LLVM 相同。

## 转换副本

新增五种 Interface 完整特化复用既有同模板逐 token 替换和模板哈希检查机制，
只改变 Clang 18 转换器看到定义的顺序。共享库保留真实接口继承构造。
`front()` 只扩展既有 indexed range 的可删除普通方法清单，不改变结构签名。
完整选中接口、头文件编辑和源文件编辑分别进入每个 current 结果的清单。
