# 本机人工复现入口

桌面目录：`/home/ce/Desktop/FramaClangFrontend-standalone-20261120`。

下面的绝对工具路径只针对当前这台电脑；可搬迁的使用方式见根 README。包内已有本机编译的前端，默认源码 constructor 已关闭。源码压缩包不含二进制，解压到其他位置时应先按 README 构建。

## 先运行前端，再与第一版原 mini 配对

以下输出目录特意留空，供人工执行；如果之前已运行过，请换一个新的目录名。

```sh
cd /home/ce/Desktop/FramaClangFrontend-standalone-20261120
python3 run.py --pass reshape \
  --llvm-prefix /home/ce/Desktop/framaclang/environment/llvm18 \
  --skip-frama --output-dir results/manual-reshape-frontend
python3 scripts/reproduce_original_mini.py --pass reshape --reshape-mini v1 \
  --source results/manual-reshape-frontend/rewritten.cpp \
  --llvm-prefix /home/ce/Desktop/framaclang/environment/llvm18 \
  --frama-c /home/ce/.opam/framac-study/bin/frama-c \
  --frama-cirgen /home/ce/.opam/framac-study/bin/framaCIRGen \
  --output-dir results/manual-reshape-v1
```

第一步预期 native_checked；第二步预期 converted、c_lines=1859。检查第二步输出的 summary.json，native_syntax、frama_conversion、frama_c_readback 的 exit_code 都应为 0，original_header_sha256 与 copied_header_sha256 应相同。

## 同一份前端输出改用第二版原 mini

```sh
python3 scripts/reproduce_original_mini.py --pass reshape --reshape-mini v2 \
  --source results/manual-reshape-frontend/rewritten.cpp \
  --llvm-prefix /home/ce/Desktop/framaclang/environment/llvm18 \
  --frama-c /home/ce/.opam/framac-study/bin/frama-c \
  --frama-cirgen /home/ce/.opam/framac-study/bin/framaCIRGen \
  --output-dir results/manual-reshape-v2
```

预期 converted、c_lines=8513。Reshape V1 头在 03_manual_passes/buddy_four/reshape/v1/mini_mlir.h；V2 头仍在 03_manual_passes/buddy_four/reshape/mini_mlir.h。两者原件都未修改。

## 另外三个 pass

先运行 run.py 生成对应 pass 的 rewritten.cpp，再把上面转换命令的 --pass 和 --source 换成对应输入。CLI 名称为 matmul-scalar、lower-ime、silu-fusion；不需要 --reshape-mini 参数。原 mini 下预期 C 行数分别为 2086、1192、3027。

不传 --source 时可直接复测 02_passes 中的已打包前端输出。注意 run.py 默认 model 模式使用共享 MiniMLIR/转换库；reproduce_original_mini.py 才是直接使用原单头文件的独立转换入口。

## 检查打包完整性

```sh
python3 scripts/check_package.py
sha256sum -c FramaClangFrontend-source.tar.gz.sha256
```

机器上已有完整测试记录：review/ctest.log。需要重跑可使用本机 CTest：

```sh
/home/ce/Desktop/BuddyCompiler/environment-v0.0.4/env/bin/ctest \
  --test-dir 01_frontend/FramaClangFrontend/build-llvm18 --output-on-failure
```
