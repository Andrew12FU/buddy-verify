# 统一前端目录：改写与接口抽象

自动运行快照已按保留策略清理；本文保留历史发现，当前通过版本见 [MiniMLIR/current](../../MiniMLIR/current)。

2026-11-16。接口抽取/抽象与兼容性改写同属 `FramaClangFrontend`；原来并列的 `Conversion/HeaderExtractionPilot` 已迁入前端，不再保留并列项目。

## 目录与职责

```text
FramaClangFrontend/
  include/
    frontend/                  改写入口配置与解析接口
    rewrite/                   修改计划
    rules/                     具体兼容性规则
    support/                   AST/源码工具与输出
    interface/                 接口抽取、观察和有界切片声明
  src/
    frontend/  rewrite/  rules/  support/  interface/
  experiment/
    *.py                       完整 pass 改写验证
    interface_pilot/
      run.py                   接口抽象先驱实验入口
      python/                  有界生成策略和验证矩阵
      fixtures/                接口抽取试验输入
      docs/                    设计、实际经验、待讨论的粒度
      results/                 原有及新实验结果
      MIGRATION.json           路径迁移及原字节哈希
      legacy_build/            原构建缓存存档，不继续运行
  tests/                       改写规则小程序回归
  docs/                        前端结构及接入记录
  build/                       两个工具共用顶层构建
  results/                     完整 pass 改写结果
  CMakeLists.txt
  run.py                       改写回归入口
```

共有13个C++头文件、16个C++实现文件，头文件和实现按职责对应组织。接口相关C++不再放在单独试验项目自己的include/src中。

顶层CMake统一发现LLVM/Clang18并构建：

- `build/buddy-framac-rewrite`：兼容性改写。
- `build/header-extraction-probe`：接口抽象先驱分析。

实验暂保留两个Python入口和各自Clang action；目录与构建统一并不意味着已经选定完整抽取粒度、合并全部分析代码或建立API契约。

## 实际迁移

| 原位置，相对于Conversion | 当前位置，相对于FramaClangFrontend |
|---|---|
| `HeaderExtractionPilot/include/*.h` | `include/interface/*.h` |
| `HeaderExtractionPilot/src/*.cpp` | `src/interface/*.cpp`；Main.cpp更名为ProbeMain.cpp |
| `HeaderExtractionPilot/run.py`、python、fixtures | `experiment/interface_pilot/`中的相应路径 |
| `HeaderExtractionPilot/docs/`、README | `experiment/interface_pilot/`中的相应路径 |
| `HeaderExtractionPilot/results/` | `experiment/interface_pilot/results/` |
| `HeaderExtractionPilot/build/`及原CMake配置 | `experiment/interface_pilot/legacy_build/`存档；当前使用顶层CMake |

迁移前后逐文件核对214个文件；其中171个历史结果文件在迁移及后续验证后仍逐字节一致。原目录已取消。详细映射和迁移时哈希见 [MIGRATION.json](../experiment/interface_pilot/MIGRATION.json)。其中源码随后进行了必要路径更新，`sha256_at_move`描述搬迁时字节，不声称路径更新后源码仍相同。

历史日志、JSON和测试副本保留当时的绝对路径，不回写为当前路径。它们用于追溯当时实验；新一轮结果已使用新路径重新生成。旧编译缓存也只作存档，不将其中绝对路径替换后继续使用。

## 路径与追溯更新

- C++ include改为`interface/Analysis.h`、`interface/Probe.h`。
- 接口Python分别定位前端根、实验根、仓库根和共享构建中的探针，不再通过旧目录层数定位pass。
- 接口实验的实现哈希包含顶层CMake、集中存放的interface C++及本实验Python/fixture。
- 改写实验的实现哈希也收录新增interface源码和先驱实验代码；显式限定源码路径，避免收录results中的历史输入、legacy_build或缓存。
- 前端README、审查指南和先驱实验复现文档更新到统一结构。

## 本轮验证

完整机读记录：整合摘要（历史自动产物已清理）。

| 验证 | 结果 | 证据 |
|---|---|---|
| 顶层统一构建 | 两个工具构建成功 | 当前build/ |
| 改写小程序回归 | 2组，20+14=34个用例通过 | CTest（历史自动产物已清理）、详细输出（历史自动产物已清理） |
| 接口抽象先驱实验 | 39条命令：37条成功/失败预期符合，2条Frama观察退出0 | 迁移后摘要（历史自动产物已清理）、命令（历史自动产物已清理） |
| 自动抽出头的内容 | 8个生成头与迁移前成功轮逐字节一致 | 整合摘要中的extracted_headers_byte_identical |
| 完整MatmulScalar+mini | 原始/改写mini、恢复真实头检查、幂等性、改写Frama转换均通过 | 完整pass回归（历史自动产物已清理） |
| 历史结果 | 171个文件内容不变 | 迁移映射及整合摘要 |
| 当前实现追溯 | 新接口实验与完整pass结果中的实现哈希均匹配当前源码 | 整合摘要 |

Matmul原始mini对照的Frama退出4仍按原始失败记录，改写后为0。完整文件的解析和转换不等于真实MLIR pass链接执行或形式化证明；本轮也未重复声称其余三个pass做了新的完整转换。

接口抽象仍处于先驱阶段；实际发现与下一步粒度讨论入口是 [FINDINGS.md](../experiment/interface_pilot/docs/FINDINGS.md)。本轮只整合层次、构建、路径和追溯，不改变已有抽取策略。
