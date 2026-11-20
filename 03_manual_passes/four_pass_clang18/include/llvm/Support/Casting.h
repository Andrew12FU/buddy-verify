#ifndef FC_LLVM_SUPPORT_CASTING_H
#define FC_LLVM_SUPPORT_CASTING_H

/* 官方 Casting.h：靠 CastInfo<To, From> 特化分派，入口模板
 * isa / cast / dyn_cast / cast_or_null / dyn_cast_or_null / isa_and_nonnull
 * 对任意 From（句柄、指针、引用）通用。
 * 模型没有实现，两个动作落到官方的两个约定上：
 *   isPossible -> To::classof(...)
 *   doCast     -> To(...)   （mlir 各句柄/op 都有从基句柄 / Operation* 的构造）
 * 因此不再需要模型私有的 castFrom。 */
namespace llvm {

template <typename To, typename From> struct CastInfo {
  static bool isPossible(const From &f) { return To::classof(f); }
  static To doCast(const From &f) { return To(f); }
  static To castFailed() { return To(); }
};

template <typename To, typename From> bool isa(const From &Val) {
  return CastInfo<To, From>::isPossible(Val);
}

template <typename To, typename From> To cast(const From &Val) {
  return CastInfo<To, From>::doCast(Val);
}

template <typename To, typename From> To dyn_cast(const From &Val) {
  return CastInfo<To, From>::isPossible(Val) ? CastInfo<To, From>::doCast(Val)
                                             : CastInfo<To, From>::castFailed();
}

template <typename To, typename From> To cast_or_null(const From &Val) {
  if (!Val)
    return CastInfo<To, From>::castFailed();
  return CastInfo<To, From>::doCast(Val);
}

template <typename To, typename From> To dyn_cast_or_null(const From &Val) {
  if (!Val)
    return CastInfo<To, From>::castFailed();
  return llvm::dyn_cast<To>(Val);
}

template <typename To, typename From> bool isa_and_nonnull(const From &Val) {
  if (!Val)
    return false;
  return llvm::isa<To>(Val);
}

} // namespace llvm
#endif
