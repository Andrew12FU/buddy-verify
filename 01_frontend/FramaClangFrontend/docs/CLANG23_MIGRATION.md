# Clang/LibTooling 23 迁移

> 2026-11-18 更新：按用户要求，前端默认也已恢复 Clang 18.1.8；下文是恢复前的历史记录，当前配置见 [CLANG18_RETURN.md](CLANG18_RETURN.md)。

后续状态：曾另行构建并将 Frama-Clang 23 设为默认，用户明确阶段分工后已恢复“frontend 23 + 转换器 18”。23 版转换器和当时证据保留为历史实验，见 [转换器实验记录](FRAMA_CLANG23_MIGRATION.md)。下面的阶段分工与当前默认配置一致。

2026-11-17。源码解析、自动改写与接口抽取默认迁移到 **Clang/LibTooling 23.1.6**。Frama-Clang 0.0.20 / Frama-C 33.0 的最终转换保留其配套 Clang 18.1.8。旧 18 二进制、人工 mini、原始 pass 和历史结果保留。

## 工具链与构建

工具链位于工作区 `environment-v0.0.4/llvm23/`，使用 LLVM 官方 Ubuntu 24.04 仓库的锁定包快照。完整包版本、SHA256 与来源见该目录 `packages.lock.json`，安装和限制见其 README。辅助库仅在工作区解压，没有系统安装。

Buddy 自身仍使用子模块 `0af6fcef7510ff068f665441564b69bcc74db3f6`（23.0.0git）；解析器开发库是官方 23.1.6 包快照，**二者不是同一提交**。这里升级的是 frontend 及其开发库，没有替换 Buddy 的真实依赖、原 pass 或生成头。

从任意目录运行 `python3 <frontend>/build_frontend.py`，默认单任务生成 `build-llvm23/`，用 Clang 23 编译并链接同套 LLVM/Clang 23 库。CMake 默认要求主版本 23；18 基线可显式使用 `--llvm-major 18` 和另一新目录重新构建。脚本拒绝覆盖历史 `build/`。

## 改动

- `run.py`、独立嵌套样例和接口抽取实验的默认工具路径切换到 `build-llvm23/`。
- `--llvm-prefix` 控制源码解析和原生检查；`--frama-llvm-prefix` 控制转换器配套环境；`--system-prefix` 指定本地已审计的 Conda sysroot/GCC 13 标准库。
- 两套解析器分别查询 `-print-resource-dir`，不再硬编码 `lib/clang/18`。Frama-Clang 参数使用其自身的 18 内建头，保持同一源文件、目标和标准库配置。
- 动态库搜索路径按进程分开。新 frontend 链接报告中的 Clang 主版本必须与原生编译器一致，否则拒绝，防止误用旧二进制。
- 适配 `PPCallbacks::InclusionDirective` 的新增参数，以及 USR 头文件迁移至 `clang/UnifiedSymbolResolution/USRGeneration.h`；兼容 18 的分支保留。
- 接口抽取实验的原生链接显式指定 Conda GCC runtime 目录。Ubuntu 的 Clang driver 不会自动读取原 Conda driver 的 GCC 安装位置，首次回归中的缺失 `crtbeginS.o` / `libstdc++` 失败现场保留。
- summary 分别记录两套工具链路径、有效原生参数、有效转换参数和各阶段退出码。

## 实测证据

迁移主目录：`results/migration-llvm23/20261117-161459/`。

- `baseline-frontend.tar.gz` 和 `baseline-source-sha256.json`：修改前 frontend 源码备份与哈希。
- `baseline-ctest.log`：旧 18 二进制的原 51 个用例全部通过。
- `rebuild-llvm18.log` / `ctest-llvm18-rebuilt.log`：迁移后的源码在新建 `build-llvm18-check/` 中用 18 重新构建成功，四组测试通过（仅 23 内建函数能力用例按版本跳过），原 `build/` 未覆盖。
- `build-llvm23.log`：首次构建的 USR 头文件路径失败现场；`build-llvm23-api-adapted.log`：适配后构建成功。
- `ctest-llvm23.log`：53 个用例全部通过，包含新增的解析器主版本一致性检查，以及 `__builtin_popcountg` 的解析、改写和执行对照。已单独确认 Clang 18 拒绝该内建函数样例。
- `comparison-with-18.json`：下表四个 pass 的源码、mini、命中数及旧二进制完整性对比。
- `nested/summary.json`：三个自包含嵌套捕获样例的改写、原生编译/执行、幂等性和 Frama-Clang 转换均通过。
- `mismatched-tool-rejected/summary.json`：使用旧 18 二进制配合新 23 配置时按版本不匹配拒绝，未进入转换成功状态。

接口抽取先驱结果：`experiment/interface_pilot/results/20261117-162823-726917/summary.json`。39 个阶段满足各自预期，包括原生执行、抽取模型的 Frama-Clang 转换，以及保留约束/删除约束的构造绑定对照；三个绑定验收条件均为 true。首次链接失败保留在 `20261117-162637-805502/`，GCC runtime 显式选择修正后新建目录重跑成功。

| 完整 pass | Clang 23 原生检查 / 幂等性 / Frama-Clang | 改写 C++ 与原 18 结果 | mini 与原 18 结果 |
| --- | --- | --- | --- |
| SimplifyTosaReshape | 全部 0 | 字节一致 | 字节一致 |
| SimplifyTosaMatmulScalar | 全部 0 | 字节一致 | 字节一致 |
| LowerIME | 全部 0 | 字节一致 | 字节一致 |
| SiLUFusion | 全部 0 | 字节一致 | 字节一致 |

四个 pass 的真实依赖检查仍针对固定 Buddy 源码及其生成头。原始 mini 的 Frama-Clang 失败是保留的对照，未作为成功阶段；实际改写输入的转换均退出 0 且新生成非空 C。

## 后续能力边界

新 parser 可以识别更多源码结构，但 Frama-Clang 18 仍有独立限制。以后遇到只能由新解析器接受的特性，需要新增经过审计的兼容改写或接口适配，使输出落在转换器能够处理的范围；没有将“Clang 23 解析成功”记作“Frama-Clang 转换成功”。

本次四个完整 pass 无须重新编写规则或 mini；现有结果仍有效。运行、链接和形式化等价结论继续受各实验原有边界限制。
