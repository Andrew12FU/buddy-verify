# Frama-Clang 转换器迁移到 Clang 23

> 2026-11-18 更新：按用户要求，前端默认也已恢复 Clang 18.1.8；下文是恢复前的历史记录，当前配置见 [CLANG18_RETURN.md](CLANG18_RETURN.md)。

当前状态：本文记录的是保留的 23 版转换器实验。用户明确 Clang 23 只负责前端改写和头文件提取后，`experiment/profiles.py` 已恢复默认使用原 OPAM 中基于 Clang 18.1.8 的 Frama-Clang；三个 Python 入口共用该配置。23 版转换器没有删除，但不再默认启用。共享 MiniMLIR 的最新回归见 [验证记录](../../MiniMLIR/docs/VALIDATION.md)。

2026-11-17。在先完成迁移前备份后，已将 Frama-Clang 0.0.20 的 C++ 转换器迁移到 LLVM/Clang 23.1.6。完整构建、补丁、回归和回退说明集中在工作区 `environment-v0.0.4/frama-clang23/README.md`。

## 实验当时的默认配置（已恢复 18 转换器）

- frontend / header-extraction-probe：`build-llvm23/`，Clang/LibTooling 23.1.6。
- C++ 转换器：`environment-v0.0.4/frama-clang23/build/framaCIRGen`，实际链接 `libclang-cpp.so.23.1` 和 `libLLVM.so.23.1`。
- Frama-C：原 OPAM 安装 33.0；OCaml 插件保持 0.0.20，中间格式 C/H 未改变。
- sysroot / GCC 13 标准库：复用旧环境的已审计文件，不用旧 Clang 解析器或内建头。

实验当时 `run.py`、`tests/run_nested_capture.py`、`experiment/interface_pilot/run.py` 均选择新版；目前均已恢复原 18。`--frama-cirgen` 与 `--frama-llvm-prefix` 分别指定转换器和配套 LLVM 环境；完整 pass 的 `run.py` 在转换前检查主版本一致性。23 版独立实验入口仍是 `environment-v0.0.4/frama-clang23/run.py`。

## 备份

`environment-v0.0.4/frama-clang23/backups/20261117-171747/before-frama-clang23.tar.gz`：1430 个文件，约 120 MB，逐成员回读哈希核对通过。包含迁移前 frontend 源码配置、原 Frama-Clang 安装和源码及 LLVM18 运行依赖。原 18 安装没有覆盖，结束时 1336 个原安装/源码/依赖文件的哈希仍一致。

## 修正范围

上游 4 个 C++ 源文件，兼容补丁 68 行（含上下文）：

1. 模板参数列表的范围接口。
2. 常量求值结果的 const 返回值。
3. 诊断颜色的新设置接口。
4. 新 AST 的 PredefinedSugarType 整数类型展开，保留目标宽度和符号。
5. RecordType 的声明选择变化：依赖登记选择完整定义，修正 CRTP 返回对象时临时变量在类定义之前出现的问题。

第 4/5 项在试编译成功后由实际转换回归发现，失败现场保留；第 5 项已有短小反例，原 18 成功、修正前 23 失败、修正后 23 成功。没有修改原 pass 或 mini 来规避这些转换器问题。补丁已在原始文件副本上重新应用，并逐字节核对可复现。

## 回归

证据总目录：`environment-v0.0.4/frama-clang23/results/20261117-172446/`。

| 回归 | 结果 |
| --- | --- |
| 四个完整 pass | `passes-verified/` 全部转换成功 |
| 四个 pass 的额外 CIL 一致性检查 | `cil-checks.json` 全部退出 0 |
| 上游 Tests OK 现存样例 | `upstream-verified/`：114 个新旧均通过，零新增失败；1 个原有 varargs 失败，另有 1 个清单文件缺失 |
| sizeof/alignof/指针差值与 CRTP 两个针对样例 | `semantics-final/`：原生 C++、旧生成 C、新生成 C 均编译执行成功 |
| frontend 规则/版本检查 | 53 个用例通过 |
| 嵌套捕获 | 3 个样例完整回归 |
| 接口抽取先驱 | 39 个阶段满足预期；最终结果 `experiment/interface_pilot/results/20261117-174818-510062/` |

上游对照使用 x86_64/C++17 的转换与 AST 一致性检查，不是完整 ptests 的所有配置矩阵。完整 pass 仍使用各自人工 mini，只验证原边界内的转换，不代表真实 MLIR 全部实现或形式化等价已验证。

## 回退

原 OPAM 转换器原位可用。在 frontend CLI 同时传入：

```text
--frama-llvm-prefix /home/ce/Desktop/framaclang/environment/llvm18
--frama-cirgen /home/ce/.opam/framac-study/bin/framaCIRGen
```

这些参数现已是默认值，无需重复传入；frontend 本身仍使用 23。以上实验及备份文件保留。
