# 条件声明：先补块，再重新解析和展开

2026-11-19：按用户要求先备份当前规则，再统一四个受支持 pass 的处理。
备份目录为工作区 `backups/if-condition-before-normalization-20261119-045015/`，
含原规则、头文件、调度配置及相关测试的 13 个文件，`manifest.json` 记录并核验 SHA-256。
原 Buddy pass、人工参考及 MiniMLIR/current 的历史通过结果不因此修改。

## 规则执行

1. `planIfConditionRewrite` 筛选启用规则、主文件及条件变量；profile 仅保留现有四种启用范围。
2. 所有 profile 均沿祖先链查找条件声明 if；被祖先替换覆盖的节点本轮跳过。
3. 验证根 if 直接位于块中，检查整条直接 else-if 条件声明链的宏、类型、临时清理和跳转边界。
4. `normalizeBranches` 给无块的 then 和非 if 的 else 包住完整语句，包括分号；
   else-if 的连接关系保留。一条链合成一项 `if-condition-normalize`，本轮不再展开该根。
5. 调用方应用补丁，重新解析输出。块已经规范化的根由 `Render` 展开为声明和显式 bool 判断，
   记录 `if-condition`。直接 else-if 递归处理，普通分支内的条件声明留到下一轮。
6. 重复到零编辑，最多 32 次，超限拒绝。多个互不覆盖的根可在同一轮处于不同阶段。

`branchText` 保护多行字面量内部字节。规范化只插入作用域块，不增加查询、
不把右侧初始化提到左条件之前。新增块不自动修改旧 AST，因此每次规范化后必须重新解析。
宏、goto/label、引用和 volatile 条件变量、非平凡析构和可观察临时清理仍拒绝。
初始化清理检查在四个 profile 中一致，不再仅按 SiLU 名字拒绝全部 ExprWithCleanups。
根 if 的块直属限制仍保留，本次不是任意控制语句位置的通用展开器。

## 调度与报告

`PassProfile.ast_rounds` 根据启用的 `if-condition` 自动选择分轮流程，包含原先单轮的 SiLU。
其他显式 `rounds` 配置保留；C++ 直接调用一次只运行一轮，不保证已经完全展开。
`rewrite_rounds()` 每轮保存输入/输出哈希、阶段和编辑数，汇总编辑携带 round 和输入哈希。
所有字节坐标均相对于该项所属轮次的输入。

历史单头模式等全部真实头文件 AST 轮次结束后才应用 include 替换。
include 编辑额外保存 original_offset，供 baseline 原文重放；offset 仍表示最终替换轮次的位置。
共享模式仍保留原 include 路径。

新增规范化项单独计数，不冒充原有条件声明展开项。专门测试覆盖四种 profile 的
单条 return、else-if 优先级、嵌套 if/else 归属、查询次数、多行字面量、统一拒绝边界、
幂等性及旧单头模式的原始坐标重放。

四个真实 pass 的源码报告已逐项核对：

| pass | if-condition-normalize | if-condition | 全部源码编辑数 |
| --- | ---: | ---: | ---: |
| silu-fusion | 0 | 1 | 10 |
| assume-tight | 1 | 1 | 9 |
| eliminate-copy | 3 | 6 | 18 |
| gpu-memcpy | 1 | 5 | 12 |

原展开项数量不变；新增项全部为分支补块。各项 before/after 和所属轮次在工作区
`audits/if-condition-normalization-20261119-045015/source-rounds/<pass>/rewrite.json`。

## 验证结果

- Clang/LibTooling 18.1.8 构建通过，全部 11 组 CTest 通过。
- 四个完整 pass 均通过原始/改写接口检查、真实头文件检查、幂等性、
  Frama-Clang 转换及生成 C 的独立回读。
- 新验证位于上述审计目录的 `full/`，GPU 的最终通过结果为 `gpu-memcpy-reviewed/`。
  较早启动的批处理仍持有旧 GPU 计数，触发了覆盖率拒绝；更新后的独立运行完整通过，
  两次记录均保留，不覆盖失败证据。
- `verification.json` 汇总备份核验、测试、四个最终通过结果及修改文件哈希。

这些检查验证当前有限模型及工具链中的行为样例和转换，不等同于一般 C++ 语义等价证明。
