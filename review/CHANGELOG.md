# 最新同步：原 mini 单头文件复现

保留现有默认规则和共享 MiniMLIR 流程不变，补入 Reshape V1 原头文件（V2 原件原位保留）、scripts/reproduce_original_mini.py 及根 README / 本机复现说明。新入口实测通过 Reshape V1/V2 及 Matmul、LowerIME、SiLU 的原 mini 组合，并按 README 两步执行了“前端生成输出 → V1 转换与回读”。详见 ORIGINAL_MINI_VALIDATION.json。

主前端实现指纹与关闭 constructor 后的九 pass 验证记录完全相同；本轮仅补资料与独立复现入口，没有切换默认头文件模型。

---

# 当前修订：源码 constructor 默认关闭

按用户确认，从 EliminateCopy 的 rules、rounds 和 expected 中移除 constructor；其余八个原本就未启用。实现与测试保留为备用，未来实际触发 CXXInheritedCtorInitExpr 时再按诊断考虑显式启用。详见 [备用规则说明](../01_frontend/FramaClangFrontend/docs/OPTIONAL_CONSTRUCTOR_RULE.md)。

关闭前的配置、EliminateCopy 产物、来源清单和源码压缩包保存在本机 before-disable-constructor/，不进入新源码压缩包。最新完整验证位于 validation/constructor-disabled/。历史人工源码及原 mini 头文件不修改。

---

# 独立仓库适配说明

本次只在桌面新目录适配；原 Buddy 中的前端、九个 pass 和历史人工版本未修改。

| 修改位置 | 之前 | 独立版 |
| --- | --- | --- |
| experiment/paths.py / profiles.py | 按 Buddy 父目录和本机绝对路径定位 | 按包根目录定位 02/03 资料；外部工具用参数、环境变量或 PATH |
| build.py / build_frontend.py | 私有环境中的 CMake/Ninja | 根入口不依赖当前目录；CMake/Ninja 可显式选择 |
| run.py / RunConfig | 固定九个工程输入和默认 current | 支持 --input、--rules、model/real；显式新输出目录 |
| compiler_arguments | 强制 Conda sysroot，文件键按当前工作目录解析 | 默认编译器标准库；可选 Conda 前缀；相对键按数据库条目目录解析 |
| --database-source | 拷贝后文件名不匹配外部数据库 | 将“获取参数的原文件”与“实际读取的输入快照”分别指定 |
| namespace-prefix | 直接按输入文件名生成 | 九个快照虽然统一叫 original.cpp，仍保留原 Buddy 名字；自定义输入使用自己的名字 |
| validation.py / artifacts.py | 必须存在 Git HEAD 和历史参考；总称真实检查 | 无 Git 可运行；自定义输入不套用样例计数；摘要区分模型与真实依赖检查 |
| ConstructorRule.cpp | 只读取已经实例化的构造默认参数 | 同时读取 Clang 保存的非依赖、尚未实例化默认参数；依赖默认值仍拒绝 |
| ExtractMiniMLIR/run.py | 默认使用本机绝对工具路径 | 复用独立版工具发现；可单独传外部参数 |
| 文档与历史探针 | 新旧运行方式混杂 | 原文移入 docs/history，早期探针移出运行包；新 README 指明维护入口 |

构造规则的改动是本次唯一 C++ 规则调整：声明型 mini 头不会像真实 MLIR 模板实现那样触发所有默认参数的实例化，因此旧读取返回空值。现在只接纳原有 portable-default 检查允许的非依赖表达式；宏、依赖表达式、名称作用域和对象身份限制仍保留。新增正反两类用例。初次打包时九个输出均与此前真实头文件模式的输出逐字节相同；之后按用户确认关闭了默认源码 constructor，当前 EliminateCopy 输出已更新。

`adaptation.patch` 包含具体实现差异；`adaptation-files.json` 额外记录未改内容的文档/历史代码移动。根入口、README 和完整性脚本为本次新增文件。`before-adaptation/` 是本机审查用初始副本，未放入源码压缩包。

`02_passes` 的原始/自动源码与专用头库来自前一轮完整验证；`03_manual_passes` 保留用户原始人工资料。关闭默认源码 constructor 后，02_passes/eliminate-copy 的自动结果及配套生成记录随验证更新；03_manual_passes 仍按原字节保留。本次重新运行的结果位于本机 `validation/model/`，最终结论见 `VALIDATION.json`。

原始验证记录中的绝对路径、历史文档和探针中的旧路径只作为历史证据。当前运行包的 Python 入口中不含 /home/ce 等开发机器路径。构建目录、运行日志与适配前快照由 .gitignore 排除；源码压缩包只包含源文件、配套资料和精简审查记录。
