# 声明和成员的提取规则

以下规则只裁剪已经抽象的共享模型。外部算法是否进入模型以及后续 ACSL 边界
见 [INTERFACE_ABSTRACTION.md](INTERFACE_ABSTRACTION.md)；提取时检查并快照
`interface-abstractions.json`，不把内部转发实现视为所有调用的必要依赖。

1. **入口**：解析改写后的单个 pass。主文件表达式引用的声明、调用的构造函数、
   使用的类型及模板、pass 基类是根。源文件仍使用原来的头文件路径。
2. **定义单位**：普通函数/成员按完整声明或完整定义选择。保留定义时不裁剪函数体
   内的语句；源模板实例映射回共享库中的模板定义。不会重新拼装函数签名。
3. **依赖闭包**：递归补齐保留部分引用的类型、别名、直接调用、构造、字段、
   默认参数及模板参数。基类同时检查语义类型和原始 TypeLoc，避免丢失依赖别名。
4. **结构单位**：保留类时同时保留基类、数据成员、类型/常量声明、构造/析构、
   复制/移动、转换运算符、虚方法及 using。这些会影响布局、可构造性、类型查询或
   override，不因当前主文件没有直接调用就删除。
5. **候选集合**：所需函数保留同名重载集合；所需类模板保留其显式/偏特化及
   前置声明。依赖表达式中暂不能确定唯一成员时，保守保留模型内同名成员集合。
   因此这是有界的保守切片，不保证数学意义上的最小集合。
6. **成员裁剪**：在上述闭包之外的完整普通类型、函数、普通成员可以删除。
   命名空间层级的 using 和前置声明保留；类内 using 随所属类进入闭包，不能单独
   将未使用的类提升为入口。宏源声明、访问标记、命名空间不做局部裁剪。宏仍来自共享
   头文件；目前没有任意宏定义内部的细粒度切片。
7. **头文件输出**：从主文件原 include 出发收集共享库的递归 include 闭包，保持
   路径和 guard，然后按 AST 字节范围删除未选声明/成员。保留文字直接取自共享库，
   不生成另一套定义。删除保留原行数，便于将诊断定位回共享头文件。

`interface-plan.json` 记录每个范围及保留理由；`interface-manifest.json` 记录
输入、共享头文件、生成头文件的哈希以及被删除的声明。专用库只允许重新生成。

## 独立调用

```sh
buddy-mini-slice rewritten.cpp --library-root /absolute/MiniMLIR/include \
  --report interface-plan.json -- -std=c++17 -I/absolute/MiniMLIR/include
python3 extract.py --source rewritten.cpp --plan interface-plan.json \
  --output /new/pass/include
```

语义选择由 `FramaClangFrontend/src/interface/MiniSlice.cpp` 完成。
Python `python/emission.py` 仅收集库内 include、验证范围和写入头文件。
