#ifndef FC_LLVM_ADT_STLEEXTRAS_H
#define FC_LLVM_ADT_STLEEXTRAS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator_range.h"

/* 官方 STLExtras.h 还有 enumerate / make_early_inc_range / zip / filter 等；
 * 四个 pass 都不用，按"只定义用到的"这里只把 iterator_range 引进来的。 */
#endif