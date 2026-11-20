# 当前通过版本

只保留 [current](../current/) 中的一份结果，每个 pass 各有一个固定子目录。
默认转换不会创建时间戳快照；成功更新，失败或仅做原生检查时恢复上一份通过版本。
显式 `--output-dir` 用于需要保存诊断的特殊运行，非空目录仍拒绝覆盖。
旧的自动转换快照已清理，原始 Buddy pass、历史人工 mini 和人工实验记录保留。

工具分工：Clang/LibTooling **18.1.8** 解析、改写和提取；Frama-Clang
**0.0.20**（配套 Clang **18.1.8**）完成转换；Frama-C **33.0** 回读生成 C。

| 完整 pass | 固定目录 | 原生检查/幂等性 | 转换 | C 回读 |
| --- | --- | --- | --- | --- |
| SimplifyTosaReshape | [reshape](../current/reshape/) | 通过 | 通过 | 通过 |
| SimplifyTosaMatmulScalar | [matmul-scalar](../current/matmul-scalar/) | 通过 | 通过 | 通过 |
| LowerIME | [lower-ime](../current/lower-ime/) | 通过 | 通过 | 通过 |
| SiLUFusion | [silu-fusion](../current/silu-fusion/) | 通过 | 通过 | 通过 |
| AssumeTightMemRefLayout | [assume-tight](../current/assume-tight/) | 通过 | 通过 | 通过 |
| EliminateMemRefCopy | [eliminate-copy](../current/eliminate-copy/) | 通过 | 通过 | 通过 |
| ConvertMemcpyToGPU | [gpu-memcpy](../current/gpu-memcpy/) | 通过 | 通过 | 通过 |
| DepthwiseConvNhwcHwc | [depthwise-conv](../current/depthwise-conv/) | 通过 | 通过 | 通过 |
| BuiltinTransposeVectorization | [builtin-transpose](../current/builtin-transpose/) | 通过 | 通过 | 通过 |

每个目录包含 `original.cpp`、`rewritten.cpp`、`converted.c`、原始提取 `include/`、
转换用 `conversion-include/`，以及本版本的补丁、改写清单、来源和验证摘要。
每个 pass 的关键日志集中在 `logs/`，其余成功阶段仅保留摘要中的命令和退出码。
重复改写和原始对照的检查副本在成功后删除。
唯一维护的完整抽象库仍在 `MiniMLIR/include`，专用库由它生成。
具体命令、退出码与哈希见各目录 `summary.json`；整体检查见
[current/verification.json](../current/verification.json)。

源码编辑数依次为 8/8/3/10/8/15/11/60/133，共 **256**；头文件兼容编辑按当前选中接口统计，详见 current/verification.json 和各 pass 的 rewrite-inventory.json。
共享模型还有一次 function_ref 抽象边界调整，单独记录，不混入转换兼容编辑数。
原始未改写源码的转换失败对照不计为通过。

## 已处理的不适配

- TypedAttr 接口模板在转换器中触发内部断言：转换副本展开同名完整特化，
  共享模型的类型身份、继承关系和原模板约束保留。
- SiLU 的继承构造：经用户允许，仅在转换副本展开 TypedAttr 和 SigmoidOp(nullptr)
  构造声明；不添加外部构造算法。
- function_ref：移除不必要的外部存储/转发实现，保留签名及模板约束，行为待 ACSL
  描述。局部 lambda 构造的空适配体不能作为无副作用契约；pass 的 lambda 体保留。
- Matmul：仅包 if 的块在纯标量比较时被转换输出展开。现于块内以具名 bool 保存
  完整条件，再 if-return；保留短路顺序，不移动外部对象或删除析构标记。
  Reshape/SiLU 的对应条件原本生成局部临时变量，故此前包块能够保留。

改写规则与完整前后文字见 [清单](../../FramaClangFrontend/docs/REWRITE_INVENTORY.md)、
[头文件兼容规则](HEADER_REWRITES.md) 和 [外部接口抽象](INTERFACE_ABSTRACTION.md)。
当前转换与回读通过不等于外部接口行为已有 ACSL 规约或已完成语义等价证明。

Clang 18 恢复、压缩包比较、新增规则与旧规则不适配的完整说明见 [CLANG18_RETURN.md](../../FramaClangFrontend/docs/CLANG18_RETURN.md)。Clang18 恢复轮次当时为九组 95 个测试：94 个通过、1 个跳过，并检查了 73 个共享头文件。最新结果见下方 BuiltinTransposeVectorization 接入回归。

BuiltinTransposeVectorization 接入后再次复测九个完整 pass：均通过转换及 C 回读。当前九组 100 个测试中 99 项通过、1 项 Clang23 专属能力检查跳过；76 个共享头文件独立检查通过，三个嵌套 lambda 的转换/回读通过。仅改压缩包头文件的对照也已通过，详见 [接入记录](BUILTIN_TRANSPOSE.md)。
