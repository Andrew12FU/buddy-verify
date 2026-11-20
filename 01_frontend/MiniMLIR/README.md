# MiniMLIR 共享接口库

本目录覆盖 SimplifyTosaReshape、SimplifyTosaMatmulScalar、LowerIME、SiLUFusion、
AssumeTightMemRefLayout、EliminateMemRefCopy、ConvertMemcpyToGPU、DepthwiseConvNhwcHwc
以及 BuiltinTransposeVectorization 所需的有界接口并集，不是完整 LLVM/MLIR，也不提供 IR 执行实现或行为契约。

唯一维护的 C++ 定义位于 `include/`，按 `llvm/`、`mlir/`、`Dialect/IME/`
组织。pass 的专用库是生成物，不允许另行修补；需要增加接口时修改这里后重新提取。
历史 `mini_mlir.h` 仅作为历史参考保留，默认运行不再使用它们。

工具分工：Clang/LibTooling 18.1.8 负责前端解析、改写和提取；最终转换由基于
Clang 18.1.8 的 Frama-Clang 0.0.20 执行。Matmul 的 guard 已在块内保存条件结果，
原先的生成 C 回读问题已消除。Matmul/SiLU 的属性模板适配和
SiLU 的继承构造展开位于转换副本；function_ref 的内部存储/转发已移出共享
接口模型。外部行为契约尚待 ACSL 接入，当前回归结果见
[VALIDATION.md](docs/VALIDATION.md)。

九个 pass 均已通过 Clang 18 正式前端改写、转换与生成 C 回读；详细结果见
[CLANG18_RETURN.md](../FramaClangFrontend/docs/CLANG18_RETURN.md)。该结论不包含尚未接入的
外部接口 ACSL 行为验证。

## 使用

在相邻 `FramaClangFrontend` 中构建工具，然后运行：

```sh
python3 build_frontend.py --jobs 1
python3 run.py --pass reshape
python3 run.py --pass matmul-scalar
python3 run.py --pass lower-ime
python3 run.py --pass silu-fusion
```

结果固定保存到 `current/<pass>/`，成功后更新，失败保留上一份通过版本，不创建时间戳快照。
仅显式 `--output-dir` 才保留额外诊断目录。嵌套捕获/接口先驱检查默认使用自动清理的临时目录。

当前版本包含 `original.cpp`、`rewritten.cpp`、`converted.c`、`include/`、`interface-plan.json` 和
`interface-manifest.json`；随后生成独立的 `conversion-include/`、
`header-rewrite.json` 和 `header-rewrite.patch`。原生检查分别覆盖两份库，最终转换
使用兼容副本。源码保留原 include 路径，模型搜索路径不回退到真实 MLIR 目录。

`buddy-mini-slice` 在**共享模型**上解析已经改写的 pass；从真实 MLIR 接口构建
抽象模型仍需要审计。这是两项不同工作，提取器不会声称已经自动抽取任意真实库。
单独使用的参数和提取规则见 [EXTRACTION.md](docs/EXTRACTION.md)。
头文件兼容改写的规则、层次和来源追踪见 [HEADER_REWRITES.md](docs/HEADER_REWRITES.md)。

## 维护边界

保留接口声明、参数/默认值、约束、基类和类型检查必需的结构。外部算法保持声明，
不把未实现函数改成返回固定值的假实现。共享原库保留真实继承构造；经用户允许，
转换副本可展开已审计的构造声明，保留原约束和默认构造，单独记录抽象影响。

外部行为后续由 ACSL 规约描述；当前没有行为契约。局部 lambda 构造等必须可见的
转换适配体单独标记，不能据其空体推断无副作用。抽象规则及 function_ref 的
具体边界见 [INTERFACE_ABSTRACTION.md](docs/INTERFACE_ABSTRACTION.md)。

来源、统一时发现的差异和现有抽象边界见 [PROVENANCE.md](docs/PROVENANCE.md)。
最终回归状态见 [VALIDATION.md](docs/VALIDATION.md)。
源码与头文件的完整规则清单及统计见 [REWRITE_INVENTORY.md](../FramaClangFrontend/docs/REWRITE_INVENTORY.md)。

BuiltinTransposeVectorization 的接口并集和 Clang 18 转换副本说明见 [接口记录](docs/BUILTIN_TRANSPOSE.md)。
