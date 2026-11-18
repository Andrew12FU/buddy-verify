# Buddy Frama-Clang Frontend（Clang 18）

当前前端、共享抽象头文件库及九个完整 pass 的通过结果，供代码审查和使用。
保留 Buddy 中的相对目录；本仓库是所选模块的快照。

- [调用和环境配置](midend/lib/Conversion/FramaClangFrontend/docs/USAGE_AND_SHARING.md)
- [前端说明](midend/lib/Conversion/FramaClangFrontend/README.md)
- [架构说明](midend/lib/Conversion/FramaClangFrontend/ARCHITECTURE.md)
- [共享 MiniMLIR 抽象库](midend/lib/Conversion/MiniMLIR/README.md)
- [九个 pass 的验证记录](midend/lib/Conversion/MiniMLIR/current/verification.json)
- [桌面下载与 Reshape 本地适配命令](local-adaptation/README.md)

## 内容与结果

`midend/lib/Conversion/FramaClangFrontend/`：C++ AST 改写、接口提取、Python 编排及测试。

`midend/lib/Conversion/MiniMLIR/include/`：唯一维护的完整抽象接口库。
`MiniMLIR/current/<pass>/`：每个 pass 的 `original.cpp`、`rewritten.cpp`、
`converted.c`、按声明/成员提取的 `include/`、转换兼容副本 `conversion-include/`、
补丁及验证日志。历史 mini 仅用于对照及输入核对。

支持的 pass：`reshape`、`matmul-scalar`、`lower-ime`、`silu-fusion`、
`assume-tight`、`eliminate-copy`、`gpu-memcpy`、`depthwise-conv`、`builtin-transpose`。

已验证工具链：Clang/LibTooling 18.1.8，Frama-Clang 0.0.20（配套 Clang 18.1.8），
Frama-C 33.0。九个完整 pass 均完成 C++→C 转换及生成 C 的 Frama-C 回读。
这些检查验证解析与转换；尚未通过 ACSL 规约证明行为等价。

## 使用前提

查看改写及生成结果可以直接浏览本仓库。重新运行完整前端时，需要匹配的完整 Buddy/LLVM/MLIR
源码、生成头文件、本机编译数据库和工具链。可以按本地适配说明，在本仓库中运行并引用
外部完整工程的真实头文件；也可以将相关目录覆盖到完整 Buddy 源码树。
来源版本见 `provenance.json`。本仓库不包含完整 Buddy、LLVM 或工具链二进制。

当前 Python 默认路径属于开发机，标准库配置采用已审计的 Conda 布局；其他机器需按
上述调用文档设置路径。普通发行版 `/usr` 工具链尚需路径适配。
验证记录内的绝对路径用于追溯原开发环境。

## 文件完整性

`upload-manifest.json` 保存从当前开发版本复制的文件及 SHA-256。
`python3 verify_snapshot.py` 检查这些文件是否完整，兼容 Git 中保留的相对符号链接。

原项目许可见 `LICENSE`；LLVM 许可见 `LICENSE-LLVM.txt`。
各源码文件原有的版权和 SPDX 声明均保留。
