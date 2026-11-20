> 当前独立版状态：下文 constructor 是实现通用化的历史记录。九个内置 pass 的默认流程现均不启用这条源码规则，实现与测试仅作备用。见 [OPTIONAL_CONSTRUCTOR_RULE.md](OPTIONAL_CONSTRUCTOR_RULE.md)。

# 剩余源码规则去除样例绑定（2026-11-20）

按规则分别备份、修改、验证和记录。pass 配置决定启用哪些能力；规则层以 AST 结构判断。
已知外部 API 的语义约束、宏/作用域/生命周期限制不作为过拟合直接删除。
原 Buddy pass、人工参考、MiniMLIR 共享库及 current 结果保持只读。

## namespace

审查结果：原实现没有 profile、pass 类名或固定变量名绑定，已经按匿名空间 canonical 声明处理。
新增外部函数用例发现旧实现只限定类类型，没有保留匿名空间的隐式 using-directive。现先声明具名空间并显式 using，再打开原定义；函数、变量和类型继续参与原包围空间的查找。重开空间复用同名。保留内部链接属性变化和外部宏引用拒绝边界。
新增九种 profile、重命名类、重开空间和外部类型引用的行为对照，输出一致。

## constructor

改前：仅 eliminate-copy、固定 EliminateMemRefCopyPattern、固定 OpRewritePattern<CopyOp> 和 MLIRContext*，无条件 explicit。
改后：识别真正的构造 using-shadow，从基类 AST 获取 public 构造签名及默认参数，生成当前派生类的完整参数转发，包括原默认值；保留 using、explicit/constexpr/noexcept 和默认构造。默认临时对象仍在调用点创建，避免缩短引用临时对象生命周期。参数不再要求指针类型或固定数量；值参数的对象身份边界见下文。
限制保留：具体非模板、无字段、单非虚基类；不合成任意转发模板、不覆盖已有用户重载。新增重命名非 MLIR 基类、尾默认参数、隐式转换、默认构造及九 profile 行为检查。

## const-local

改前：限定 depthwise-conv/builtin-transpose 和 mlir::Value/AffineExpr。
改后：所有启用 profile 使用同一规则，接纳具名、平凡可复制且平凡析构的 record，完整限定模板实参；仍初始化一次并以原名 const 引用访问。
新增保守拒绝：所在函数的 decltype、decltype(auto) 返回或 const_cast、constexpr/属性声明。保留宏、volatile、块直属和生命周期限制。重命名模板类型、只读重载及初始化次数在九 profile 对照执行。

## range-for

原实现本身无 profile/pass 名字绑定。补齐普通范围循环的数组和可命名 ADL begin/end：从 Clang 已选择的调用声明生成限定调用，防止普通名字查找选择别的重载。原成员 begin/end、auto&& 生命周期和 early_inc 表达式保持。
llvm::enumerate + ArrayRef 的下标展开仍保留：它依赖已知轻量视图和代理语义，不能把任意容器复制后按下标遍历。写下标、引用逃逸、隐藏 friend、宏、跳转等检查不是样例绑定。新增数组引用修改和与普通 lookup 冲突的 ADL 用例，在九 profile 对照执行。

## init-list

改前：两个 profile 使用不同的列表查找/父路径策略；TypeRange、若干元素类型、只读元数据调用仅在 transpose 打开，SmallVector 固定 OpFoldResult/容量3。
改后：所有 profile 使用自身实参列表、所有父路径检查和由内到外重新解析；支持类型及 SmallVector 容量由 AST 取得。数组元素接纳算术/枚举/对象指针和平凡可复制 record。
保留 ValueRange/TypeRange/ArrayRef/SmallVector 的库类型身份，因为输出调用依赖这些 API 的范围构造契约；已审计的 MLIR 不可变查询及 LLVM ArrayRef/StringRef 等只读视图构造白名单保留，未知调用及用户有副作用构造仍拒绝。所有 profile 均使用同一白名单，另拒绝未求值祖先。新增自定义平凡元素、容量8和九 profile 行为对照。

## value-range

改前：固定 SiLU、GenericOp::create、至少8参数、第3/4号两个列表、第7号callback、siluInput/Output 生成名。
改后：根据实参 AST 的 ValueRange 类型发现任意数量和位置的列表，处理重命名的普通/成员直接调用；允许块内表达式或单自动变量声明，使用中性名字。其他实参按局部引用、字面量或无初始化捕获的轻量回调检查，不再固定 callback 位置。
保留 Value/EmptyOp 的已审计转换契约以及作用域/求值/空列表/宏限制；未知自定义 Value 转换不能仅凭名称相似放行。新增单列表和三列表、不同参数位置/函数名、九 profile 行为对照，原有两列表与 capture 插入顺序测试保留。

## while（复核）

没有 profile、pass、类名或变量名绑定；不改算法。新增九 profile 的每轮初始化、continue 和重载否定行为对照。保留非平凡清理/引用/volatile/跳转/宏边界。

## if-condition（遗留限制补齐）

删除规则层剩余的四 profile 白名单；所有启用者共用补块、fresh-AST 展开、祖先所有权和生命周期检查。原默认启用集合不扩大。把已有分支归属、查询时机、字面量及拒绝测试扩展到九 profile。

## 集成与参考

九个原 pass 的默认规则集合不扩大。Matmul 继续强制逐字节参考检查：新建 structural-rules 参考，只在既有 generic-guard 参考中独立补入已审查的 namespace 显式 using；旧参考保持不变，没有关闭或放宽匹配。
每条规则的独立源码备份、前后 SHA-256、完整 patch 和局部验证写入 audits/remaining-rules-generalization-*/<rule>/；最终完整 pass 另存 verified/，full/ 和 full-final/ 保留阶段证据，不会覆盖 MiniMLIR/current。

集成边界复查补充：构造参数用 Clang 类型打印器保留函数指针等声明符；对已有带默认参数的用户重载不插入竞争重载；歧义基类重载拒绝。完整签名转发以临时引用默认值存活到整个调用表达式结束为行为回归。ValueRange 的其他实参检查隐式非平凡构造，并拒绝可能在构造时执行回调的未知包装器。

## constructor：去除最后的指针调用形状绑定

完整参数/default 转发后不再要求首参数为指针或恰有一个必需参数。支持具体非默认、非复制/移动构造的完整签名，参数名统一生成 argument。用户已有构造的 owner 保持不动，避免增加竞争重载；有默认尾参数形成歧义的基类签名前缀拒绝。保留单非虚基类/无字段/类型可命名/安全值参数或引用/可搬移默认拼写等语义前提；安全值参数范围见下文。新增普通 int/double 构造、默认值和 noexcept 的九 profile 对照。此前指针版验证产物保留作阶段证据，最终验证另行记录。

## AST 类型打印复查

列表元素不再把 long 强行拼成 int64_t。数组声明通过 Clang 类型打印器插入声明符，支持函数指针等复杂指针类型；枚举循环计数类型来自绑定声明的实际无符号整数类型，不再假设全局 size_t 存在。新增无 int64_t/全局 size_t 别名以及函数指针列表的九 profile 编译执行对照。输出类型身份不变，拼写与依赖要求更准确。

构造值参数的额外复制边界：即使 record 平凡可复制，prvalue 中保存的自身地址仍可让额外参数副本变得可观察。通用转发只接纳标量/指针/枚举等值参数和引用；record 值参数限于已审计 MLIR OpRewritePattern 的 PatternBenefit/ArrayRef 值语义协议。它不绑定具体 pass 或 op 特化。新增自指针 record 拒绝用例，避免把“平凡复制”误作对象身份无关的证明。
