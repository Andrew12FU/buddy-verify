# 通用 if + || 改写

2026-11-19：guard 从 pass 定制的 failure guard 改为基于 AST 结构的运行期 if 改写。
规则不查询 profile、类名、方法名、返回函数、返回值或分支语句数量。
profile 仅决定完整 pass 默认是否启用 `guard`；显式启用后各 profile 使用同一算法。

```cpp
if (a || b) {
  act();
  finish();
} else fallback();
```

改写结构为：

```cpp
{
  bool guardFailed = (a || b);
  if (guardFailed) {
    act();
    finish();
  } else fallback();
}
```

`src/rules/GuardRule.cpp` 只替换条件文本，完整保留 then/else 的原始字节，
因此不假定分支为 return，也不改变注释和多行字面量内容。原条件仍是一个
完整表达式：保留内建 || 的求值顺序、短路和临时对象清理时机；重载 ! 等叶子
操作不重建。新 bool 通过主文件名字清单避让。

带初始化语句的 `if (init; a || b)` 将 init 放入同一个新增外层块，在 bool
初始化之前执行；初始化对象仍覆盖条件与两个分支，并在整条 if 结束后销毁。
循环内或没有外部花括号的 if 也适用，break/continue/return 归属保持。

## AST 轮次及边界

- 匹配 `IfStmt` 的条件，略过检查所需的括号、隐式转换和清理包装，确认根为
  `BinaryOperator` / `BO_LOr`。输出仍使用完整原条件。
- 外层 if 负责本轮替换；内层、else-if，以及位于其源码内的 lambda 条件
  留到下一轮。`PassProfile.ast_rounds` 与 `rewrite_rounds()` 为 guard 启用
  fresh-AST 收敛检查，最多 32 轮；原文偏移按各轮输入分别记录。
- 新 if 的条件变为具名 bool，因此不会再次命中；最终幂等性检查仍要求零编辑。
- 重载 ||、非 || 条件、`if constexpr` 不匹配。宏位置及 goto/label 引入的
  作用域风险会拒绝整份计划。这些是结构/语义边界，不是 pass 白名单。
- 原始 pass、真实头文件和已有 current 产物不由本次侧聊天覆盖。

## 已审阅的完整源码计数

| pass | 旧 guard 数 | 新 guard 数 | 新增命中 |
| --- | ---: | ---: | --- |
| reshape | 1 | 1 | 无；输出统一使用 bool |
| matmul-scalar | 6 | 7 | batch 维度循环中的动态形状判断 |
| silu-fusion | 1 | 1 | 无；带块分支使用相同算法 |
| eliminate-copy | 1 | 2 | 最后 stride 的普通计算分支，不返回 failure |

Matmul 的严格参考更新为 `tests/references/matmul-scalar-generic-guard.cpp`，
保留旧 `matmul-scalar-guard-binding.cpp` 和历史手工快照。规则数量与参考比较
仍是独立验收条件，没有关闭检查。

## 回归

新增 `tests/test_guard.py` 并接入 CTest，覆盖九个 profile 下相同行为、普通类与
方法、循环、任意 then/else、嵌套和 else-if 收敛、if-init 对象生命周期、
条件临时对象清理、lambda 条件、字面量字节保留、幂等性及拒绝路径。
原先断言 profile/方法/return 白名单的测试改为验证通用匹配，同时保留短路
次数、顺序、名字冲突和行为对照。

构建与回归在独立目录 `/tmp/buddy-generic-guard-20261119-054702` 执行，
12 个 CTest 测试组通过。上述四个完整 pass 均通过原生检查、真实依赖检查、
接口副本检查、零编辑幂等性、Frama-Clang 转换和独立 C 回读，状态均为 converted。
Matmul 新严格参考逐字节匹配。独立诊断输出位于
`/tmp/buddy-generic-guard-full-20261119-054702/`，各阶段记录见各自 summary.json。
没有使用这些诊断结果替换共享的 MiniMLIR/current。

本次开始后补存的备份说明位于工作区
`backups/frontend-before-generic-guard-20261119-054702/README.md`：
其中本聊天首先修改的四个文件恢复为改前内容，其余内容为补存时快照，
并记录了重建来源和逐文件校验。它不冒充修改前已创建的原子快照。
