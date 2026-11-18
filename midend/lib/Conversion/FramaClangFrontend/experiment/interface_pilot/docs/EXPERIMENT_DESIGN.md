# 接口抽象先驱实验：设计与复现

## 要回答的问题

1. “被调用的公开成员”是否足够生成可解析的头文件？
2. 仅声明与保留短实现的模型，分别能支持解析、链接、常量求值和转换中的哪些步骤？
3. 删除模板约束是否会改变合法输入集合及所选重载？
4. 宏、私有字段、别名、继承、隐式构造和默认实参应如何进入粒度决策？
5. 当前静态 mini 在完整 pass 上与真实接口有什么可观察差异？

## 统一前端构建

本地已安装的配套 Clang/LLVM 是18.1.8；所观察 Buddy 源树的 LLVM 是另一个版本。不能把两个版本的头文件或库互换使用。

```bash
FRONTEND=/home/ce/Desktop/BuddyCompiler/buddy-mlir-v0.0.4-git/midend/lib/Conversion/FramaClangFrontend
LLVM_PREFIX=/home/ce/Desktop/framaclang/environment/llvm18
"$LLVM_PREFIX/bin/cmake" -S "$FRONTEND" -B "$FRONTEND/build" -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$LLVM_PREFIX/bin/ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$LLVM_PREFIX/bin/clang" \
  -DCMAKE_CXX_COMPILER="$LLVM_PREFIX/bin/clang++" \
  -DLLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm" \
  -DClang_DIR="$LLVM_PREFIX/lib/cmake/clang"
"$LLVM_PREFIX/bin/cmake" --build "$FRONTEND/build" --parallel 1
python3 "$FRONTEND/experiment/interface_pilot/run.py"
```

`--output-dir` 只允许指定本实验 `results/` 下尚不存在的目录。每轮保留参数、当前实现/二进制哈希、指定输入哈希、每条实际命令、stdout/stderr及退出码。未建立所有传递系统头的封闭环境快照；结果只适用于已记录的本地配置。

运行依赖包括 Frama-C/Frama-Clang、真实编译数据库和现有 Matmul mini。`python/environment.py` 固定其路径并自行提取编译参数；接口策略与改写验证暂分别维护。两个C++工具共用前端顶层构建。

## 真实 AST 观察

- `src/interface/Inventory.cpp`（相对于前端根） 只遍历主文件声明及方法体，记录实际选中的 CallExpr 和 CXXConstructExpr、canonical签名、参数数、默认实参数、模板来源、宏位置、部分类型性质及override边。
- `src/interface/Probe.cpp` 记录主文件include及宏事件；为 ArrayRef 额外记录 `LLVM_LIFETIME_BOUND` 的真实预处理替换。
- `src/interface/LibrarySlice.cpp` 从命名空间查找两个指定primary类；保存真实成员源码及源文件位置，保留模板壳和约束。短函数体的直接调用、构造和字段引用形成有限依赖边。
- Clang USR只用于当前版本内关联声明；不宣称能跨版本、宏配置或不同类型抽象保持稳定。

这不是所有类型、全部继承、间接调用、所有隐式析构与模板候选的完整依赖分析。

## LogicalResult 对照

根为真实 `llvm::failure(bool)` 与 `llvm::failed(LogicalResult)`。

| 变体 | 自动生成方式 | 检查 |
|---|---|---|
| surface | 两个根函数及其直接调用到的成员，保留体 | 原生解析，观察未闭合私有依赖 |
| closure | 在已收集成员内递归跟踪调用/构造/字段引用 | 解析、链接、执行、类型性质、Frama转换 |
| declarations | 将闭包函数改为声明并删除字段 | 同上，明确检查预期失败 |
| real | 直接include真实头文件 | 原生基准 |

闭包遇到未知依赖边会失败，而不是自动创建空函数体。生成的类壳仅针对当前无基类的 LogicalResult。未自动推导或添加requires/ensures/assigns。

## ArrayRef 对照

固定选择默认构造、单元素构造、指针长度构造、受约束容器构造、size/data/operator[]、Data/Length和size_type。

这个成员集及类型别名/字段的必要性是本轮人工限定的实验策略；成员源码、约束、参数类型、来源与寿命属性宏替换由真实 AST/预处理器取得。标准库依赖以人工列出的四个真实头文件提供，不声称完成标准库抽取。

| 变体 | 改变的因素 |
|---|---|
| raw | 源码切片直接拼装，不提供寿命属性宏定义 |
| closed | 保留所选字段、短实现、原模板约束与真实寿命宏替换 |
| no_fields | 在closed基础上删除字段，仍保留方法体 |
| declarations | 保留字段/模板约束，函数体改为声明 |
| unconstrained_declarations | 仅将容器构造人为替换成无约束的模板声明 |

最后一项是明确的人工消融，不是声称等价的自动修复。测试ScalarLike的用户自定义转换及Unrelated负例，比较实际构造绑定与合法输入集合。另有constexpr对象初始化、索引/长度/别名运行对照。默认构造之外未测试全部特殊成员性质。

第一轮真实ArrayRef的运行输入因ABI检查符号未解析而链接失败。后续保留同样的不链接库对照，并另行链接Buddy配套 `libLLVMSupport.a`。没有定义假符号，没有混用Clang18配套LLVM库替代真实Buddy依赖。

## 完整 pass 对照

1. 原始MatmulScalar在真实依赖下观察。
2. 原文只替换include为已有人工mini，保持每行数量，观察同一调用点。
3. 原始LowerIME在自己的真实编译参数下观察。

Matmul比较键包括行列、调用/构造类别、实际声明所属名称、canonical签名、结果类型、参数数和默认参数数。同位置可能有多个隐式节点，因此按多重集比较。差异清单只用于审阅，不自动判定等价或错误，不把继承压平的不同成员名称自动合并。

## 判据

`pilot_completed` 表示：本轮预先登记的成功/失败对照与绑定变化断言均满足。它不表示所有用例成功，更不表示整个MLIR模型或pass被验证。

命令预期为failure时，信号退出或超时不算预期语法失败。Frama转换先记录为observe；阅读实际退出码与非空C文件后再描述观测结果，转换不等于链接、执行或形式化证明。
