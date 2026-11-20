# include/ 官方化迁移：改了什么、还剩哪些强制偏差

以 MLIR/LLVM `release/21.x` 为基准，把 `include/` 全面改成官方形状。
验证：`su - analyst` + `frama-c 33.0`（Frama-Clang 0.0.20），四个 pass 全部
`convert_exit=0`、`out.c` 回读 0 错误（脚本：`audit/run_all.sh`）。

| pass | out.c 行数 | 转换 | 回读 |
| --- | --- | --- | --- |
| assume | 5599 | ✅ | ✅ |
| eliminate | 6649 | ✅ | ✅ |
| depthwise | 9102 | ✅ | ✅ |
| gpumemcpy | 8084 | ✅ | ✅ |

改前的整棵树备份在 `include.bak_20261117/`（58 文件），四个源码各有
`*.frama.cpp.bak_20261117.bak`。

## 一、已按官方改掉的（要点）

- **句柄**：`Type`/`Attribute`/`Value`/`Operation`/`Block`/`Region`/`Dialect`/`MLIRContext`
  去掉 `int kind`/`int reserved`，改用官方 `impl` 指针字段；补 `getTypeID()`/`getContext()`、
  `operator==/!=/bool/!`；`TypeID` 移到官方路径 `mlir/Support/TypeID.h`。
- **判别与转换**：删掉 `enum TypeKind/AttributeKind/ValueKind`、`getKind()`、
  `classofOp`、`castFrom`；改为官方 `static bool classof(...)` +
  `llvm::CastInfo<To, From>`（`isa/cast/dyn_cast/cast_or_null/dyn_cast_or_null/isa_and_nonnull`
  全是泛型模板，`To::classof` + `To(...)` 两个约定）。
- **范围类型**：`use_range`/`user_range` = `llvm::iterator_range<use_iterator/user_iterator>`；
  `ResultRange`/`OperandRange`/`MutableOperandRange`/`ValueRange`/`TypeRange` 按官方在
  `ValueRange.h` / `TypeRange.h` 里声明；`Operation::getOpOperands()` 返回
  `llvm::MutableArrayRef<OpOperand>`；`Block::BlockArgListType` 是类内 `MutableArrayRef`。
- **Operation**：索引/计数改 `unsigned`（`getOperand/getResult/getNumOperands/getNumResults/setOperand`），
  `emitOpError(const Twine & = {})` 返回 `InFlightDiagnostic`，`walk` 返回 `WalkResult`；
  `Twine` 在官方路径 `llvm/ADT/Twine.h`，`InFlightDiagnostic` 在 `mlir/IR/Diagnostics.h`。
- **Op 类**：全部改成 `class XxxOp : public Op<XxxOp>`（官方 `OpState` 提供
  `operator Operation*`/`operator bool`/`getOperation`/`getContext`/`getLoc`/`erase`/`emitOpError`/`walk`），
  `create(...)` 返回 **op 对象**（不再返回 `Value`/`void`），并补 `static StringRef getOperationName()`。
  单结果 op → `Value` 的隐式转换由 `Op<>::operator Value()` 提供（官方由 `OneTypedResult` trait 提供）。
- **Pass 层**：`PassBase` → `Pass`；`OperationPass<OpT>` 改成官方类模板（含 `void` 特化）；
  `PassWrapper<PassT, BaseT>` 第二参改成**基类**；`Option` 的父对象类型改 `Pass &`、
  第 2 参改 `StringRef`；`PassRegistration` 移到官方路径 `mlir/Pass/PassRegistry.h`，
  `MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID` 移到 `mlir/Support/TypeID.h`。
- **Greedy / Conversion**：`GreedyRewriteStrictness` 改 `enum class`（含 `ExistingOps`）；
  `GreedyRewriteConfig` 改"私有字段 + 链式 setter"（`setStrictness` 返回自身引用）；
  `applyPatternsGreedily(Operation *, const FrozenRewritePatternSet &, config = {}, bool * = 0)`；
  `ConversionPattern`/`ConversionPatternRewriter` 移到 `DialectConversion.h`；
  `applyPartialConversion(Operation *, const ConversionTarget &, const FrozenRewritePatternSet &, ConversionConfig = {})`。
- **支持类型**：`LogicalResult` 按 21.x 落在 `llvm/Support/LogicalResult.h`（mlir 侧
  `typedef` + 转发 `success/failure`）；`WalkResult` 移到 `mlir/Support/WalkResult.h` 并删掉
  非官方的 `operator bool`；`function_ref` 移到官方路径 `llvm/ADT/STLFunctionalExtras.h`
  （`FunctionRef.h` 只保留为转发）；`ArrayRef` 加官方通用容器构造与非成员 `==/!=`；
  `SmallVector` 用官方的 `CalculateSmallVectorDefaultInlinedElements<T>::value` 默认值；
  `CommandLine` 的 `desc`/`init` 改官方（`desc(StringRef)`、`init` 是函数模板）；
  `ErrorHandling` 补 `report_fatal_error(StringRef)` + `llvm_unreachable_internal`；
  `MutableArrayRef` 按官方放在 `ArrayRef.h`。

## 二、仍然偏离官方的地方（每条都有原因）

### A. Frama-Clang 0.0.20 硬限制

1. `llvm/Support/Compiler.h` 里保留 `int64_t`/`size_t`/`fc_size_t` typedef：
   前端自带的标准库没有 `<cstdint>`/`<cstddef>`。
2. `mlir/Support/LLVM.h` 用 `typedef`/别名模板代替官方的 `using llvm::X;`
   （前端报 `Unsupported Type (Using)`）；`mlir::isa/cast/dyn_cast/...` 写成同名转发模板。
   `std::move` 返回左值引用、`std::nullopt_t/nullopt` 由模型提供（没有 `<utility>`/`<optional>`）。
3. `llvm::function_ref` 按 arity 展开成 6 个偏特化（前端实例化不了参数包），
   构造按值接收（官方是 `Callable &&` + SFINAE）；每个特化带一个 `int reserved;`
   （否则生成空 C 结构体，Frama-C 默认 machdep 拒绝）。
4. `ValueRange` 额外提供 2/3/4 个 `Value` 的显式构造：官方只有 initializer_list，
   而本前端不接受变量声明里的花括号列表（源码 20 余处依赖这几个重载）。
5. `scf::ForallOp::create` 的 mapping 形参收 `std::nullopt_t`（官方是
   `std::optional<ArrayAttr>`；模型没有 `<optional>`，也没有定义 `ArrayAttr`）。
6. `vector::CreateMaskOp::create` 第 2 参收 `ValueRange`（官方是 `ArrayRef<OpFoldResult>`）：
   调用点传的是单个 Value。
7. `RewritePatternSet::add` 按三个固定元数声明（`add<T>()` / `add<T>(A0)` / `add<T>(A0,A1)`）：
   官方是变参模板，本前端会生成重名形参（`redefinition of 'args'`）。
8. `Pass::Option` 是**命名空间级** `mlir::Option`（官方是 `Pass::Option` 嵌套类）：
   前端在"嵌套类模板 + 成员初始化列表"上直接断言失败（`Clang_utils.h:892`）；
   其构造参数与 `operator DataType()` 仍按官方。构造按 4 参固定形态
   （官方是变参 `(Pass&, StringRef, Args&&...)`，变参会断言）。
9. `FrozenRewritePatternSet(RewritePatternSet &)`（官方 `&&`）：`std::move` 替身返回左值。
10. 迭代器类（`use_iterator`/`user_iterator`/`ElementIterator`）内部用裸指针实现
    （官方是 `iterator_adaptor`），`begin()/end()` 的参数与语义一致。
11. `ArrayRef`/`SmallVector` 的成员**只声明不实现**：Frama-Clang 会实例化全部成员，
    指针运算要求元素完整（`SmallVector<OpFoldResult>` 会报 incomplete pointer）。
    为避开空结构体，`pattern`/`op` 等基类各自保留了官方本来就有的字段。
12. `Op<>` 额外提供默认构造 `Op() : OpState(0) {}`：官方没有，
    但 `CastInfo::castFailed()` 与 `Value::getDefiningOp<T>()` 需要 `T()`。
13. `vector`（无扩展名）仍是 `std::vector` 的替身（前端没有 `<vector>`）。
14. `IntegerSet`/`MLIRContext`/`Location` 仍是"带 `int reserved` 的壳"：
    它们只作为类型出现，没有语义；官方对应的完整类在四个 pass 里一个成员都没用到。

### B. 路径层面

- `mlir/Dialect/Linalg/IR/LinalgOps.h` 官方 18.x 起没有（内容已并入 `Linalg.h`），
  模型保留该文件只为让 pass 的 `#include` 原样可用。
- `mlir/Pass/Pass.h` 额外 include 了 `PassRegistry.h`/`TypeID.h`（官方由用户自己 include），
  目的是不改动 pass 源码的 include 列表。

## 三、连带改动的四个 pass 源码（语义不变）

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| assume | `func.begin()/end()` → `func.getBody().begin()/end()` | 官方 FuncOp 没有 begin/end |
| assume / eliminate / gpumemcpy | `use_range`/`user_range` 的迭代器**先声明**、`end()` 先存变量，再用 `for (; it != itEnd; ++it)` | 前端不接受 for-init 里的类类型变量与条件里的临时量 |
| gpumemcpy | `MutableOperandRange` → `llvm::MutableArrayRef<OpOperand>`；`copyOp.getOperand(n)`→`copyOp->getOperand(n)`；`gpuAllocOp.getResult(n)`→`gpuAllocOp->getResult(n)` | 官方 op 类只转发 OpState 成员，`getOperand/getResult(n)` 在 `Operation` 上 |
| depthwise | `forOp.getResult(0)`→`forOp->getResult(0)`；`getStrides().getValues<int64_t>()[0]` → 迭代器逐个取；`VectorType::get(vecSize, T)` → `VectorType::get(ArrayRef<int64_t>(&vecSize,1), T)` | `getValues` 返回 `iterator_range`（不可下标）；官方 `VectorType::get` 只收 `ArrayRef<int64_t>` |

## 四、回归方式

```bash
# 在 WSL 里（frama-c 在 analyst 的 opam 环境）
su - analyst -c 'bash /mnt/d/DDKKK/deepseekworkplace/non-game/total/audit/run_all.sh'
```
