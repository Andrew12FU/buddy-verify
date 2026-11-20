# 四个 Pass 的 Frama-Clang 限制

环境：Frama-C 33.0 + Frama-Clang 0.0.20。

## AssumeTightMemRefLayout

### 1. range-based for（8 处）
- 出现：`for (Block &block : func)`、`for (Value v : worklist)` 等。
- 报错：`Unsupported Statement: CXXForRangeStmt`
- 改法：改 `begin/end` 或下标循环。
  ```cpp
  for (auto it = r.begin(); it != r.end(); ++it) { auto &x = *it; ... }
  ```

### 2. 结构化绑定（2 处）
- 出现：`for (auto [idx, dim] : llvm::enumerate(type.getShape()))`
- 报错：`Unsupported reference expression: ... Binding ...`
- 改法：显式下标 + 普通变量。
  ```cpp
  auto shape = type.getShape();
  for (int64_t i = 0; i < (int64_t)shape.size(); ++i) {
    int64_t dim = shape[i];
  }
  ```

### 3. 匿名命名空间（1 处）
- 出现：`namespace { ... }`
- 报错：`[fclang] Failure: could not find all needed definitions`；带虚函数时 `Unknown global variable anonymous_namespace_1::...::_frama_c_vmt_header`
- 改法：改具名命名空间，类外定义和 `PassRegistration` 用限定名。

### 4. lambda 按引用捕获类对象（1 处）
- 出现：`func.walk([&](Operation *op) { worklist.push_back(...); })`
- 报错：`implicit cast: cast from struct _ZN4llvm... to struct ... *`
- 改法：先取指针，再按值捕获。
  ```cpp
  SmallVector<Value> *wp = &worklist;
  func.walk([wp](Operation *op) { wp->push_back(...); });
  ```

## EliminateMemRefCopy

### 1. range-based for（5 处）
- 报错：`Unsupported Statement: CXXForRangeStmt`
- 改法：`begin/end` 或下标循环。

### 2. 结构化绑定（2 处）
- 报错：`Unsupported reference expression: ... Binding ...`
- 改法：`auto [idx, dim]` 拆成两个普通变量。

### 3. 匿名命名空间（1 处）
- 报错：`could not find all needed definitions`
- 改法：改 `namespace eliminate_memref_copy_detail`，限定类外定义和 `PassRegistration`。

### 4. if 条件声明 + 用户定义 bool（6 处）
- 出现：`if (auto x = dyn_cast<...>(...))`
- 报错：`cannot cast expr of type struct _Z... into a boolean value`
- 改法：先声明再 `if`，建议加花括号保持作用域。
  ```cpp
  {
    auto x = dyn_cast<...>(...);
    if (x) { ... }
  }
  ```

### 5. 复合 `||` 条件 + 类类型局部量（1 处）
- 出现：`if (!sourceType || !destType) return failure();`
- 报错：`cannot jump from goto statement bypassing initialization of variable ...`
- 改法：拆成两个 `if`（或先算 `bool`）。

### 6. 继承构造函数（1 处，潜伏）
- 出现：`using OpRewritePattern<memref::CopyOp>::OpRewritePattern;`，一旦真正构造对象。
- 报错：`Unsupported expression: CXXInheritedCtorInitExpr`
- 改法：显式转发构造。
  ```cpp
  explicit EliminateMemRefCopyPattern(MLIRContext *context)
      : OpRewritePattern<memref::CopyOp>(context) {}
  ```

## DepthwiseConvNhwcHwc

### 1. 匿名命名空间（2 处）
- 报错：`could not find all needed definitions` / `Unknown global variable ..._vmt_header`
- 改法：改 `depthwise_conv_detail`、`depthwise_pass_detail`，限定类外定义和 `PassRegistration`。

### 2. lambda 按引用捕获类对象（7 处）
- 出现：`[&]` 捕获 `Value`、`VectorType`、`Type`、`AffineExpr` 等。
- 报错：`implicit cast: cast from struct ... to struct ... *`
- 改法：类对象先取指针，lambda 按值捕获指针，体内用 `(*p)` / `p->`。

### 3. 嵌套 lambda 跨级捕获（7 处）
- 出现：内层 lambda 捕获外层函数变量，或内层完全不捕获。
- 报错：`Cannot find field __fc_lambda_overload_0 in type struct ...`
- 改法：每层把需要的东西落成**直接外层 lambda 的局部量**，再让内层捕获；内层无捕获时加一个哑捕获。或拆成同级静态函数并通过状态结构体传参。

### 4. 变量声明处的花括号初始化列表（28 处）
- 出现：`ValueRange{...}`、`VectorType::get({vecSize}, ...)`、`SmallVector<...>({N, OH, OW})`
- 报错：`redefinition of 'init_array' in the same scope` / `redefinition of 'init_list' ...`
- 改法：
  ```cpp
  ValueRange(OC)
  VectorType::get(vecSize, type)
  SmallVector<OpFoldResult, 3> v; v.push_back(N); ...
  ```

### 5. `const T x = <返回对象>`（6 处）
- 出现：`const Value c0 = arith::ConstantOp::create(...);`
- 报错：`Cannot assign to non-modifiable lval c0`
- 改法：
  ```cpp
  Value c0Init = arith::ConstantOp::create(...);
  const Value c0 = c0Init;
  ```

## ConvertMemcpyToGPU

### 1. range-based for（3 处）
- 报错：`Unsupported Statement: CXXForRangeStmt`
- 改法：`begin/end` 或下标循环。

### 2. if 条件声明 + 用户定义 bool（9 处）
- 报错：`cannot cast expr of type struct ... into a boolean value`（或解析失败）
- 改法：先声明再 `if`；`if/else if` 链上的 `dyn_cast` 提前求值。

### 3. lambda 按引用捕获类对象（2 处）
- 出现：`funcOp->walk([&](Operation *nestedOp) { ... builder ... })`
- 报错：`no viable conversion` / `implicit cast: struct ... to struct ... *`
- 改法：捕获指针 `builderPtr`、`unDeallocatedValuePtr`、`funcOpPtr`，体内用 `(*p)` / `p->`。

### 4. 匿名命名空间（1 处）
- 报错：`could not find all needed definitions` / `Unknown global variable ..._vmt_header`
- 改法：改 `convert_memcpy_to_gpu_detail`，限定类外定义和 `PassRegistration`。


