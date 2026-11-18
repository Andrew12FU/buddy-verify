# 前端的接口抽取与抽象先驱模块

本模块属于 [FramaClangFrontend](../../README.md)，与源码兼容性改写共同组成前端。C++ 已归入统一的 `include/interface/` 与 `src/interface/`，顶层CMake同时构建两个工具；本目录保留先驱试验的Python策略、样例、经验和结果。

**当前实际自动抽取的范围**：真实LLVM LogicalResult的短函数依赖闭包；真实ArrayRef的固定成员切片、源码位置及宏替换快照。完整MatmulScalar / LowerIME仅观察真实AST；Matmul的完整mini对照使用既有人工模型，不能称为自动抽取完整pass成功。

先读 [实际结果与粒度因素](docs/FINDINGS.md)，再看 [实验设计和复现](docs/EXPERIMENT_DESIGN.md)。统一目录映射及验证见 [前端整合记录](../../docs/INTERFACE_LAYOUT.md)。

```text
FramaClangFrontend/
  include/interface/               接口分析的C++声明
  src/interface/                   接口探针和有界抽取实现
  experiment/interface_pilot/
    python/                        模型生成、工具链边界、实验矩阵
    fixtures/                      真实LLVM头文件的最小调用入口
    docs/                          经验与抽象粒度讨论
    run.py                         先驱实验入口
    MIGRATION.json                 原目录→统一目录的迁移与字节哈希
    results/<时间戳>/              历史及新实验，拒绝覆盖
    legacy_build/                  迁移前编译缓存留档，不继续使用
  build/header-extraction-probe    顶层构建产物
```

在前端根目录完成统一构建后运行：

```bash
python3 /home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend/experiment/interface_pilot/run.py
```

历史结果文件搬迁时逐字节核对，不改写其中的旧绝对路径和命令。它们记录原位置；迁移后的实际位置见MIGRATION.json。新实验使用当前路径，不依赖旧位置存在。

当前没有任意API行为契约推断、完整类型/实现依赖闭包或完整MLIR头生成。接口抽象与改写在同一前端内分层发展，实验入口分别保留。
