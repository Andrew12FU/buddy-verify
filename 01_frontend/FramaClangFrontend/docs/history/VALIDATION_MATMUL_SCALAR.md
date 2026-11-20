# 第二轮：SimplifyTosaMatmulScalar 自动改写验证

日期：2026-11-13，Asia/Shanghai。新增 MatmulScalar 配置，并复测共享工具上的 Reshape。

## 输入、配置与范围

- 原始源码：`../GraphRedundancyElimination/SimplifyTosaMatmulScalar.cpp`。
- 人工参考与 mini：`../GraphRedundancyElimination/test_framaclang/simplify_tosa_matmul_scalar/full_pass/steps/100_complete_pass/`。
- 依据：该 pass 的 `full_pass/rewrite.md`；本轮不修改该文件、人工最终输入、mini 或真实头文件。
- 配置：独立 Clang/LibTooling 18.1.8；本机 Frama-C 33.0 / Frama-Clang 0.0.20；读取 MatmulScalar 自己的真实编译数据库条目。
- 与当前 Reshape runner 一致，使用 gnu++17、无异常/RTTI、x86_64-conda-linux-gnu、已审计 sysroot/GCC 标准库搜索参数及 `-cxx-nostdinc`。实际命令完整保存。
- 不增加接口提取功能，不合并两个 pass 的 mini，不增加新的构造或列表实参改写。

最终运行目录：[matmul-scalar/20261113-191744-133582](results/matmul-scalar/20261113-191744-133582/)。首次成功试运行亦保留于独立目录；以下以最终运行结果为准。

## 新增实现

`run.py --pass matmul-scalar` 选择独立源码、参考输入、mini、启用规则和命中计数。C++ 通过 `--profile=matmul-scalar` 选择 guard 和 include 范围；重复改写使用同一 profile。

MatmulScalar guard 需同时满足：

1. 位于 `ReplaceScalarLikeMatmul::matchAndRewrite` 方法体的直接子级；按 AST 所属关系判断，不使用行号或字符串替换。
2. 条件根节点是内建逻辑或；then 为直接返回 `mlir::failure` 或 `llvm::failure` 的语句。
3. 不带 else、初始化语句或条件声明。

仅在完整 if 外新增块，不改条件、实参、短路顺序或返回。循环内判断和已包块的 if 不满足直接方法体子级关系，因此保持原样且具有幂等性。其他类、其他方法、重载 `operator||` 不应用此规则。

## 实际命中

共 16 处源码编辑，不是 16 个语言特性或独立 bug。

| 规则 | 编辑数 | 内容 |
| --- | ---: | --- |
| interface-substitution | 8 | 真实解析完成后，将本 pass 的八个已知 MLIR includes 换为 mini |
| namespace | 1 | 匿名空间具名化 |
| namespace-reference | 1 | 注册处 pass 类型加限定 |
| guard | 6 | 六个完整 if 分别加外层块 |

六个 guard 的位置以原始文件为准：

| 原行号 | 条件 |
| --- | --- |
| 50 | 三个 RankedTensorType 句柄有效性 |
| 53 | 三个 rank 至少为 2 |
| 60 | rank 一致性 |
| 77–79 | 六个尾部维度的静态性 |
| 82 | K_lhs、K_rhs、J 等于 1 |
| 84 | 输出 I、J 一致性 |

**自动生成的完整 C++ 与历史人工最终 `input.cpp` 逐字节一致。** Runner 已将此独立参考比较列为 MatmulScalar 的验收条件；结果保存在 `reference_source_byte_equal`。因此本次没有改动循环内 guard、`{1}`、默认 encoding、查询/属性/创建/替换调用或算法体。

## 实测结果

| 阶段 | 结果 |
| --- | --- |
| C++ 前端解析原始源码及真实依赖、执行改写 | 退出 0 |
| 原始源码仅替换 includes 后的 mini 对照 | 原生检查退出 0 |
| 改写结果 + mini | 原生检查退出 0 |
| 改写结果恢复真实 includes | 原生检查退出 0 |
| 人工参考对照 | 完整 C++ 逐字节一致 |
| 对结果再次改写 | 退出 0；0 处编辑；源码逐字节一致 |
| 原始 mini 对照的 Frama-Clang 转换 | 退出 4，完整失败日志保留 |
| 改写结果的 Frama-Clang 转换 | 退出 0；新生成 C 为 121428 字节 |
| 输入保护 | 原始 pass、mini、人工参考输入运行前后 SHA-256 一致 |
| 实现追溯 | 最终摘要记录的 C++、Python、测试和二进制哈希与执行版本一致 |

现有 mini 的未定义 inline 等警告仍完整保留，没有将退出 0 当成无警告。

本次生成 C 与历史 C 不逐字节一致：文本对照发现新增一段 `std::move` 声明及其打印出的 requires/ensures 注解。当前命令与历史实验不同，本轮未作单因素原因定位。自动改写器没有向输入 C++ 或 mini 新增契约；上述逐字节一致结论只针对输入 C++。

## 回归与 Reshape 复测

原 16 个回归用例保留，新增 4 个，共 **20 个 unittest 用例全部通过**，由一个 CTest 测试组执行。

新增检查覆盖：

- 六组 guard 中每个短路叶子导致失败的路径，以及全部不失败的路径；核对求值次数和顺序。
- 循环内部的三项 guard 不改写，完整程序前后运行结果相同。
- MatmulScalar 的第二次改写无编辑。
- 其他方法/类、带 else 或初始化的 if、非 failure 返回和重载 `||` 保持原样。
- MatmulScalar profile 不套用 Reshape 的独立 guard 匹配。
- 八个已知 include 被替换，无关头文件保留。

Reshape 最终复测目录：[reshape/20261113-191744-148413](results/reshape/20261113-191744-148413/)。原有 13 处编辑计数不变，三组原生检查通过，第二次改写 0 编辑，Frama-Clang 退出 0，生成 C 722342 字节；输入哈希保持不变。其 C++ 与人工参考的表达形式差异仍按第一轮说明保留，不要求逐字节一致。

Python 编译检查、C++ 格式检查及增量构建通过。没有链接或执行完整 MLIR pass，没有开展 Frama-C 形式化证明。

## 审阅产物

- [完整补丁](results/matmul-scalar/20261113-191744-133582/rewrite.patch)
- [自动生成的 C++](results/matmul-scalar/20261113-191744-133582/rewritten.cpp)
- [逐项改写记录](results/matmul-scalar/20261113-191744-133582/rewrite.json)
- [阶段、命令、参考比较和哈希](results/matmul-scalar/20261113-191744-133582/summary.json)
- [完整转换日志](results/matmul-scalar/20261113-191744-133582/frama_rewritten.log)
- [生成 C](results/matmul-scalar/20261113-191744-133582/converted.c)

结论限定为：MatmulScalar 的已有人工改写已由前端自动复现，在本次固定配置和已有 mini 下可转换；源码也通过真实依赖的原生检查。接口抽象和外部实现覆盖范围沿用历史记录，不扩展为完整 MLIR 实现或一般语义等价结论。
