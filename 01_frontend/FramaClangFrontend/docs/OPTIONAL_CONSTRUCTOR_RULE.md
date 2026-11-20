# 备用源码规则 constructor：当前九个 pass 不启用

状态：源码规则 `src/rules/ConstructorRule.cpp` 保留实现、命令行名称和专门测试，作为显式启用的备用兼容规则。当前九个内置 pass 的 rules、分轮顺序和预期编辑数均不包含 constructor，默认输出不增加源码级的转发构造函数。

## 原代码及当前为何不需要改

EliminateCopy 的原写法是合法 C++：

```cpp
struct EliminateMemRefCopyPattern : public OpRewritePattern<memref::CopyOp> {
  using OpRewritePattern<memref::CopyOp>::OpRewritePattern;
  // matchAndRewrite ...
};
// runOnOperation 中：
patterns.add<EliminateMemRefCopyPattern>(ctx);
```

Matmul、Reshape、SiLU 同样通过 using 继承 OpRewritePattern 的构造函数。不能根据具体 pass 名字判断这种语法必须展开。当前声明型 MiniMLIR 中，RewritePatternSet::add 只有声明，未展开真实 add/create/分配函数体；以上注册调用不会像完整真实实现那样沿函数体构造 pattern。当前这些 pass 的模型转换路径无需启用此源码兼容规则。

此前 EliminateCopy 单独启用 constructor，是从历史人工参考继承的潜在兼容安排，不是当前九个 pass 都已触发的错误。现在关闭这项默认安排，并在关闭后重新验证。

## 未来什么情况下可能出错

继承构造声明本身没有错误。若未来输入直接构造派生对象，或头文件模型加入并实例化真实 add/create/分配实现，Clang 可能生成实际继承构造的 CXXInheritedCtorInitExpr。历史 Frama-Clang 0.0.20 / Clang 18 对这种表达式曾报 Unsupported expression: CXXInheritedCtorInitExpr；原历史包也将它标为“潜伏，一旦真正构造对象”。不能据此断言任意继承构造或其他版本转换器都必然失败。

遇到此错误，应先从诊断定位具体对象类型和触发构造的调用。只有问题属于源码中这条规则支持的继承构造形状时，才考虑手动启用 constructor；它不支持任意模板构造、重载组合或生命周期语义。

## 显式启用方式

例如将 EliminateCopy 的当前默认规则补上 constructor：

```sh
python3 run.py --pass eliminate-copy \
  --rules namespace,constructor,range-for,if-condition,guard \
  --llvm-prefix "$LLVM_PREFIX" \
  --skip-frama --output-dir results/eliminate-constructor-opt-in
```

这会关闭该样例固定编辑数及参考文本验收，仍执行规则的拒绝条件和其余检查。去掉 --skip-frama 并指定 Frama 工具，才能检验是否解决实际转换问题。

## 之前打包时的读取错误是另一回事

该备用规则读取基类构造的默认参数时，旧实现只调用 getDefaultArg()。声明型 mini 头中，Clang 可能尚未实例化默认参数，必须通过 hasUninstantiatedDefaultArg()/getUninstantiatedDefaultArg() 读取其保存的非依赖表达式。这个读取遗漏已经修复；依赖模板参数、宏、不可搬移的默认值和不安全的值参数仍拒绝。保留的正反用例验证规则自身能力，不表示九个 pass 的默认流程使用了它。

## 与头文件改写的区别

MiniMLIR 的转换兼容阶段另有 typed-attr-constructor-expansion 等头文件规则，它们处理库类型而非 pass 源码中的 pattern 派生类。这次关闭的是源码 constructor 规则；头文件转换规则保持原有行为，不能把报告中其他 constructor-expansion 名称误读为此源码规则仍在运行。
