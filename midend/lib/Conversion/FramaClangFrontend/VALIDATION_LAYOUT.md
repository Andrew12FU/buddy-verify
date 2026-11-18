# 目录统一与完整 pass + mini 回归

日期：2026-11-13，Asia/Shanghai。

## 当前代码目录

- `include/`：全部 6 个 C++ 头文件。
- `src/`：全部 10 个 C++ 实现文件，包含命令行、公共模块和四个规则；平铺存放，不再有 `rules/` 子目录。
- 根目录不再放置实现用 `.h` / `.cpp`。测试运行生成的 pass 副本仍按轮次存入 `results/`，与工具实现区分。
- CMake 源列表、私有头文件搜索路径、Python 实现哈希清单和当前审查指南已同步调整。

本轮只迁移目录与引用路径，没有改变规则。原始 Buddy 文件、所用 mini 和历史人工输入不修改。

## 完整 pass 回归实际做了什么

本轮执行以下两个入口，没有使用 `--skip-frama`：

```bash
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/run.py --pass reshape
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/run.py --pass matmul-scalar
```

每轮输入是**整份 pass 源文件与该 pass 对应的已有 mini 头文件**，不是最小片段或只保留一个方法的外壳。工具先解析原始真实依赖，再输出完整改写副本；Python 不裁剪方法体。

每轮分别检查：

1. 仅替换 include 的原始完整 pass + mini 能否通过原生语法/语义检查。
2. 完整改写 pass + mini 能否通过原生检查。
3. 同一改写 pass 恢复真实 includes 后能否通过原生检查。
4. 对完整改写文件再次执行，是否零新增编辑且字节一致。
5. 原始完整 mini 对照的 Frama-Clang 结果，以及完整改写输入的转换结果和新生成的非空 C。

## 本轮结果

| 检查项 | Reshape | MatmulScalar |
| --- | --- | --- |
| 原始完整 pass + mini 原生检查 | 0 | 0 |
| 完整改写 pass + mini 原生检查 | 0 | 0 |
| 完整改写 pass + 真实头文件原生检查 | 0 | 0 |
| 完整改写 pass + mini 的 Frama-Clang 转换 | 0 | 0 |
| 新生成 C 大小 | 722342 字节 | 121428 字节 |
| 第二次改写 | 0 编辑，源码字节不变 | 0 编辑，源码字节不变 |
| 与迁移目录前的完整改写 C++ 对照 | 逐字节一致 | 逐字节一致 |
| 原件与实际实现哈希 | 原件未变，实现/二进制与本轮摘要一致 | 同左 |

此外，20 个规则小程序回归用例全部通过。它们与上述完整文件回归是两个验证层次，不能互相替代。

原始完整 mini 对照的转换仍分别退出 4；这与原生检查通过并不矛盾。完整日志保存既有模型的警告，没有将改写后退出 0 宣称为无警告。

## 可直接核对的完整文件和证据

| Pass | 完整 C++ | 对应 mini | 阶段结果 |
| --- | --- | --- | --- |
| Reshape | [rewritten.cpp](results/reshape/20261113-194831-116825/rewritten.cpp) | [mini_mlir.h](results/reshape/20261113-194831-116825/mini_mlir.h) | [summary.json](results/reshape/20261113-194831-116825/summary.json) |
| MatmulScalar | [rewritten.cpp](results/matmul-scalar/20261113-194831-116824/rewritten.cpp) | [mini_mlir.h](results/matmul-scalar/20261113-194831-116824/mini_mlir.h) | [summary.json](results/matmul-scalar/20261113-194831-116824/summary.json) |

完整 pass + mini 原生检查日志：[Reshape](results/reshape/20261113-194831-116825/native_rewritten_mini.log)、[MatmulScalar](results/matmul-scalar/20261113-194831-116824/native_rewritten_mini.log)。

完整 Frama-Clang 转换日志：[Reshape](results/reshape/20261113-194831-116825/frama_rewritten.log)、[MatmulScalar](results/matmul-scalar/20261113-194831-116824/frama_rewritten.log)。

生成 C：[Reshape](results/reshape/20261113-194831-116825/converted.c)、[MatmulScalar](results/matmul-scalar/20261113-194831-116824/converted.c)。

这里的“完整 pass 回归”限定于完整文件的原生检查和 Frama-Clang 转换，尚未链接执行真实 MLIR pass。mini 继续沿用各自既有模型，本轮没有自动提取新头文件，也没有扩展外部实现覆盖或证明一般语义等价。
