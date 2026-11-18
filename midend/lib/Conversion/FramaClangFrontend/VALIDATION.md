# 第一轮：SimplifyTosaReshape 自动改写验证

日期：2026-11-13，Asia/Shanghai。只验证本 pass，不将结论推广到其他 pass。

## 固定输入和环境

- 原始源码：`../GraphRedundancyElimination/SimplifyTosaReshape.cpp`。
- 接口：复用 `../GraphRedundancyElimination/test_framaclang/simplify_tosa_reshape/full_pass_v2/final/mini_mlir.h`，未修改。
- C++ 前端：Clang/LibTooling 18.1.8；独立构建，不链接 Buddy 的 LLVM 23 构建库。
- 转换：本机 Frama-C 33.0 / Frama-Clang 0.0.20，`framaCIRGen` 配套 Clang 18。
- 配置：真实编译数据库条目，gnu++17、无异常/RTTI、x86_64-conda-linux-gnu；沿用已审计 sysroot 与 GCC 标准库头文件配置。
- 本轮不修改原始源码、真实库、mini 或既有人工实验。

正式最终复跑目录：[20261113-190117-172396](results/reshape/20261113-190117-172396/)。此前失败定位和中间成功运行也保留于 `results/reshape/`，不作为本轮最终产物。

## 验证结果

| 阶段 | 实测结果 |
| --- | --- |
| C++ 前端解析原始源码及真实依赖、生成计划 | 退出 0 |
| 原始源码仅替换 includes 后的 mini 对照 | 原生检查退出 0 |
| 自动改写源码 + mini | 原生检查退出 0 |
| 同一自动改写源码恢复真实 includes | 原生检查退出 0 |
| 对自动结果再次改写 | 退出 0；0 项修改；源码逐字节一致 |
| 原始 mini 对照的 Frama-Clang 转换 | 退出 4；完整日志保留，未作为成功产物 |
| 自动改写结果的 Frama-Clang 转换 | 退出 0；新生成 C 为 722342 字节 |
| 原始源码、mini、人工参考输入保护 | 运行前后 SHA-256 一致 |
| C++ / Python 实现与工具追溯 | 摘要中的源码、二进制哈希与本轮执行版本一致 |
| 回归测试 | 16 个 unittest 用例全部通过，作为一个 CTest 测试组运行 |
| 其他检查 | Python 编译检查、C++ 格式检查通过；构建无新增警告 |

转换日志仍包含既有模型的未定义 inline、异常假设等警告；没有将退出 0 写成“无警告”。

## 实际规则覆盖

共 13 处源码编辑，分为以下规则；编辑数不是语言特性支持率。

| 规则 | 编辑数 | 说明 |
| --- | ---: | --- |
| interface-substitution | 5 | 替换五个已知 include；真实头文件已先参与解析 |
| namespace | 1 | 为匿名命名空间命名；保留链接属性变化的说明 |
| namespace-reference | 1 | 注册处的 pass 类型限定到新命名空间 |
| while | 1 | 每轮重新查询，显式 bool 判断，保留原循环体嵌套作用域 |
| capture-pointers | 2 | 为两个闭包分别插入指向原对象的指针局部变量 |
| capture | 2 | 改写 op/base 和 reshapeOps 的捕获及使用 |
| guard | 1 | 为完整的 `if (!inTy || !outTy) return failure();` 增加外层块 |

自动结果沿用已有人工方案的结构，但不追求与人工文件逐字节一致：

- while 使用 `!static_cast<bool>(inner)`，避免改成否定表达式后意外选择重载 `operator!`；保留原循环体作为内层块，以保持局部变量遮蔽关系。
- 指针对象使用写作 `(*opPtr).method()`，按 AST 声明身份替换引用；不按变量名字全局替换。
- 真实 MLIR 的 guard 使用重载 `operator!`，`failure` 的真实声明位于 LLVM；规则同时识别真实接口与当前 mini 的对应结构。
- 匿名空间内部的宏生成类型引用保留，不对不可编辑的宏展开直接打补丁。

继承构造保持原声明；本轮没有启用新的转发构造规则，也没有扩大接口实现覆盖范围。

## 回归覆盖

行为对照检查：每轮条件求值、continue、条件变量遮蔽、布尔转换与重载否定的区别、原捕获对象的修改、同名局部变量、生成名称冲突、匿名空间重开/类外定义/外部类型引用、宏内部类型、短路求值，以及真实接口的重载否定和 failure 别名。

拒绝路径检查：非平凡析构、初始化临时对象清理、成员/自由重载取地址、宏中的捕获使用、嵌套闭包、相互覆盖的规则、无效 C++。另检查通过符号链接覆盖原文件会被拒绝。

这些小程序的前后运行对照只覆盖相应规则样本。完整 MLIR pass 没有链接或执行，也未进行 Frama-C 形式化证明。

## 可审阅产物

- [完整改写补丁](results/reshape/20261113-190117-172396/rewrite.patch)
- [自动生成的 C++](results/reshape/20261113-190117-172396/rewritten.cpp)
- [逐项改写记录](results/reshape/20261113-190117-172396/rewrite.json)
- [阶段结果、命令和哈希](results/reshape/20261113-190117-172396/summary.json)
- [转换日志](results/reshape/20261113-190117-172396/frama_rewritten.log)
- [本次生成的 C](results/reshape/20261113-190117-172396/converted.c)

本轮结论：**原始 Reshape 经自动执行已知改写，在当前已有接口模型及固定配置下可转换；改写结果同时通过真实依赖的原生检查。** 尚未自动提取头文件，未证明一般语义等价，未验证真实 MLIR 外部算法实现。
