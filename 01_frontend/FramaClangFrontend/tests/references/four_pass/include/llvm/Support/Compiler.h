#ifndef FC_LLVM_SUPPORT_COMPILER_H
#define FC_LLVM_SUPPORT_COMPILER_H

/* Minimal compiler/type shim: the model only needs fixed-width integers. */
typedef __INT64_TYPE__ int64_t;
typedef __SIZE_TYPE__ fc_size_t;
typedef __SIZE_TYPE__ size_t;
#endif
