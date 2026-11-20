# 通用 capture 规则

`capture` 的候选和改写不读取 profile。profile 只负责在 runner 中启用规则。
`CaptureRule.cpp` 统一委托 `NestedCapture.cpp` 的捕获树规划器；单层 lambda
是仅有根节点的树。原来的 BuiltinTranspose 专用 BorrowedCapture 分发已合并。

## 统一处理的三个维度

- 捕获内容：按引用捕获的类对象、标量、引用变量、数组及指针变量，统一保存
  被引用存储的地址，按值捕获指针；类型检查使用 getNonReferenceType。
- 所在位置：所有 profile 接受同块直接表达式和单个自动局部变量初始化。
  不跨短路、条件表达式、静态初始化或其他控制流边界提取声明。
- 嵌套关系：最外层统一收集捕获树，按 canonical VarDecl 区分对象；
  内层将继承地址复制为自己的局部指针再按值捕获，避免后端的捕获字段解析问题。
  内层新出现的局部对象或参数在其所在调用体内取地址。

## 值捕获不转换成原对象指针

```cpp
int n = 1, result = 0;
auto f = [n, &result]() { result = n; };
n = 2;
f();  // result 必须为 1
```

改写：

```cpp
int n = 1, result = 0;
auto *resultPtr = &result;
auto f = [n, resultPtr]() { (*resultPtr) = n; };
n = 2;
f();
```

普通值捕获保留名称及捕获顺序，保留副本、复制构造、const/mutable 行为；
纯值捕获且子树没有候选时不编辑。嵌套中若父层按值捕获对象，子层按引用捕获，
指针在父层体内指向父闭包的副本，不能沿用函数中原对象的地址。
父层已把同一变量转为引用指针而子层再按值捕获的情形，当前明确拒绝：
需要另外实现初始化捕获的后端适配，不能把指针值当作对象副本。

## this

只支持通过 this 访问普通、非 volatile、非位域的整数字段。
保存当前实例的字段地址，替换对应 MemberExpr；嵌套闭包逐层传播字段地址。
方法调用、直接传递 this、限定基类成员访问以及参与改写树中的 [*this] 拒绝。
这是 AST 使用方式边界，不再限定 BuiltinTranspose。

## 其他明确边界

参与改写树中的初始化捕获、捕获包、泛型/局部类、宏、未求值上下文、
重载取地址、register 变量、decltype(auto) 返回值、含 goto/label 的插入块、
以及需要把自身名字提前到声明前的自捕获等仍拒绝。
`decltype(auto)` 中 `return x` 与 `return (*xPtr)` 的推导结果不同，不能直接替换。
拒绝时不输出部分改写。原对象的生命周期不延长，未证明闭包逃逸后的使用有效。
同一捕获树先合成内部补丁，再提交一个根替换，避免父子编辑重叠。

## 验证

`tests/test_capture.py` 检查九个 profile 的输出一致、标量/引用变量候选、
混合捕获、对象副本及复制构造次数、数组、volatile 访问、嵌套 this 的实例身份、
不支持结构和二次改写零编辑。`test_nested_capture.py` 检查多层地址传播、
返回闭包、局部对象、同名遮蔽，以及父层副本的身份。原有 SiLU 测试中的
“其他 profile 拒绝声明初始化”已改为按统一结构接受。

修改前快照位于工作区 backups/frontend-before-generic-capture-20261119-064937。
完整验证记录在同目录 generic-capture-validation.json。
旧 NESTED_CAPTURE.md、IME_SILU.md、BUILTIN_TRANSPOSE.md 中的 profile 边界是历史记录，
当前捕获规则以本文和源码为准。

最终实测：默认 Clang 18 前端已重新构建，13 组 CTest 全部通过；
Reshape、SiLU、AssumeTight、GPU Memcpy、Depthwise、BuiltinTranspose 六个完整 pass
均完成原生检查、真实依赖检查、幂等性检查、Frama-Clang 转换及独立 C 回读。
六次运行的源码指纹和工具哈希与当前默认前端一致。完整产物保留在独立验证目录，
未替换 MiniMLIR/current；摘要、测试日志和本次改动 patch 保存在修改前备份目录。
