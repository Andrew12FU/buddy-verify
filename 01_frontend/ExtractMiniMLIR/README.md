# ExtractMiniMLIR：从真实使用生成粗粒度接口骨架

这是重新开发的独立生成器。输入是 rewritten.cpp、真实 LLVM/MLIR 头文件与编译环境；
**没有读取、裁剪、复制或补入既有 MiniMLIR 声明的步骤**。不覆盖共享库。
目前生成链路可用于下述简单案例，TosaMatmul 候选仍未通过编译，不能作为已验证的 pass 使用。

## 结构与阅读顺序

`include/` 与 `src/` 中的职责对应：

- `model/Registry`：以 Clang USR 区分类型、重载和实例化；记录状态、实际使用、成员来源。
- `analysis/Collect`：只遍历主文件声明及表达式，包括隐式表达式、lambda 体、override 要求；不遍历库的普通实现体。
- `analysis/Relations`：读取真实 CastKind、选中操作及基类路径；识别仅用于成员接收者的基类转换。
- `generation/Policy`：有界模板特化支持与明确的空 walk 策略；不按 pass 名称选择生成规则。
- `generation/Emit`：命名空间、前置声明、定义顺序、方法签名、字段、别名及 using 导出。
- `driver/Main`：Clang Tooling 入口，另行观察主文件直接写出的宏调用。
- `run.py`：分阶段运行现有改写器、真实解析、提取、候选检查、转换及 C 回读。
- `python/diagnostics.py`：归类编译错误，关联真实声明与使用点；错误文本不用于生成声明。

只复用 FramaClangFrontend 的编译数据库参数准备、工具链环境与进程执行工具。
生成声明的 C++ 模块不链接旧探针、旧 slicer 或共享 MiniMLIR。

## 当前自动生成的范围

1. 从 TypeLoc、变量和表达式类型注册实际所需类型；指针/引用允许仅前置声明，
   按值对象及成员访问使状态升级。类型记录区分“需要的状态”与“实际生成状态”。
2. 依据实际选中的 FunctionDecl 生成普通函数、方法、构造/转换声明及实际访问的字段。
   保留参数/返回类型、const/volatile、ref 限定、static、访问权限和 explicit。
   虚方法 override 要求单列收集；常规 noexcept 可保留，依赖表达式单列未支持。
3. 基类方法允许放到当前接收者骨架中。分别记录 real_owner 和 generated_owner；
   纯接收者基类转换被这种落位消解，不为它再创建 trait/基类骨架。
   实参中的独立继承转换仍单列为未支持，未生成任意类型互转。
4. 类、枚举、源文件直接使用的类型别名分别处理。普通枚举暂保守保留其枚举项。
   签名使用规范类型，避免为了内部 typedef 再引入 type_traits 实现。
   对主文件名称查找实际经过的 using shadow，记录并输出对应 using。
5. 类型参数均为无约束、无默认值、无参数包的类模板，可生成 primary 前置声明和
   **实际观察到的显式特化骨架**，只含用到的成员；未生成完整通用模板实现。
   新增未观察到的实例化不保证可用。其他模板有具体未支持记录，不变成普通 class/重载。
6. 隐式构造、复制/移动、赋值调用、转换、bool 上下文以及临时对象/局部对象析构均记录。
   隐式特殊成员允许由抽象骨架的 C++ 隐式规则产生；它不是对真实布局/析构行为的恢复。
7. 原头文件路径由很薄的转发头导入独立 skeleton.h；只重定向输入的字面 include 路径，
   不复制宏定义文件或真实头文件实现。

所有生成接口的外部行为仍未规定，尚无 ACSL、对象状态模型或语义等价证明。

## Callback 边界

当前仅对 MLIR 成员中满足 `template<class Callback> void walk(Callback[&&/&])`
这一无约束、单回调参数形状的接口生成空模板定义，逐次记录真实声明 USR 和规则。
其他 callback 不会自动清空。更复杂的 walk 返回类型、约束或多参数签名仍不支持。

输入中的 lambda 体保持原样。**空 walk 不执行 callback，也不保证 lambda 体出现在生成 C 中。**
Matmul 输入没有 walk，本轮其结果不依赖这一规则。

## 本地复现

以下命令使用本机现有环境。先构建，后运行；同一输出目录会替换上一次本生成器拥有的候选。

```bash
W=/home/ce/Desktop/BuddyCompiler
C="$W/buddy-mlir-v0.0.4-git/midend/lib/Conversion"
"$W/environment-v0.0.4/env/bin/cmake" --build "$C/FramaClangFrontend/build-llvm18" --target buddy-extract-minimlir -j1
python3 "$C/ExtractMiniMLIR/run.py" \
  --source "$C/GraphRedundancyElimination/SimplifyTosaMatmulScalar.cpp" \
  --compile-commands "$W/build-v0.0.4/buddy/compile_commands.json"
```

已存在 rewritten.cpp 时，在该命令加 `--input /绝对路径/rewritten.cpp`，将原样复制输入，
不再次改写。输入不得放在将被替换的输出目录内部。`--source` 仍用于选取真实编译数据库条目。
默认使用 Clang 18.1.8；转换器为 Frama-Clang 0.0.20（配套 Clang 18.1.8）。
运行入口会移除真实 include 搜索路径后再检查生成库，避免缺漏被原 MLIR 头文件填补。

运行四个简单案例，并额外执行基础案例的转换及 C 回读：

```bash
python3 "$C/ExtractMiniMLIR/tests/test_skeleton.py" \
  --tool "$C/FramaClangFrontend/build-llvm18/ExtractMiniMLIR/buddy-extract-minimlir" \
  --clang /home/ce/Desktop/framaclang/environment/llvm18/bin/clang++ \
  --frama-c /home/ce/.opam/framac-study/bin/frama-c \
  --converter /home/ce/.opam/framac-study/bin/framaCIRGen
```

如已有配置目录不存在，应按现有前端 build_frontend.py/README 的方式配置 Clang 18 构建环境。
提取器本身也支持 `buddy-extract-minimlir rewritten.cpp --output-dir DIR -- <真实编译参数>`。
返回 0 只表示真实 AST 解析和文件写出成功；库是否可编译由后续阶段判断。

## 本轮结果与未完成部分

四个简单案例：成员增量及真实隐式操作、有限模板特化、空 walk 明确边界、复杂模板拒绝。
全部通过。基础案例的 Clang、Frama-Clang 转换和生成 C 的回读均通过。

TosaMatmul 已完成原 pass 改写、真实环境 Clang 检查、自动记录和候选生成。
候选库 Clang 检查和 Frama-Clang 转换未通过；没有成功生成 C，因此未执行 C 回读。
目前不能把它称为 TosaMatmul 最小可用头文件库。

剩余阻塞包括：

- `llvm::dyn_cast`：选中的是函数模板，须保留对应模板参数、约束及推导所需定义；当前没有制造普通重载。
- `RewriterBase::replaceOpWithNewOp`、`RewritePatternSet::add`、`DialectRegistry::insert`、`std::move`：
  同样缺少函数模板生成/定义规则，具体使用点见记录。
- `OperationPass` 与 `basic_string`：类模板默认参数尚未支持，所以不是完整模板骨架；
  后者通过 FrozenRewritePatternSet 构造默认实参所用的 `ArrayRef<std::string>` 签名引入。
- `IntegerType::SignednessSemantics`：嵌套枚举需要在 IntegerType 内安排声明/定义。
- `IntegerType::get`、`RankedTensorType::get`、`failure/success`、FrozenRewritePatternSet 构造等：
  尚未重建默认实参表达式，不通过删参数来绕过。
- `ShapedType::isDynamic`、`StringRef`/`ArrayRef`/Type/GreedyRewriteConfig 等 constexpr 接口：
  当前尚未区分、重建必需的有界定义，明确列为未支持。
- 独立实参的继承转换，包括 PatternRewriter→OpBuilder、IntegerType→Type 等：
  已记录真实路径；尚未合成对应抽象转换支持。
- 输入中的 `MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID`：已记录主文件宏调用；
  未复制其宏实现，也没有把它定义为空来隐藏缺漏。

这些是生成器能力缺口。当前检查尚不能把失败归因于 Frama-Clang 自身限制。

## 当前产物

`current/tosa-matmul/` 只保留一份当前候选：

- `rewritten.cpp`、`rewrite.json`：现有改写器原样输出；提取阶段没有编辑它。
- `include/skeleton.h` 与转发头：仅从真实 AST 记录生成。
- `records.json`：类型状态、接口来源/使用点、实际转换和关系、未支持项。
- `unsupported.json`：未支持项与可追溯的使用点、依赖理由；候选错误分类。
- `summary.json`、`commands.json`、`logs/`：各阶段真实退出状态、可重放 argv、必要日志。

转换失败时不会留下伪装成可用结果的 C 文件。测试的临时文件在测试结束后清理。
