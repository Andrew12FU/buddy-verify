#ifndef FC_LLVM_ADT_STLFUNCTIONALEXTRAS_H
#define FC_LLVM_ADT_STLFUNCTIONALEXTRAS_H

/* 官方 `llvm::function_ref<Ret(Params...)>` 在 llvm/ADT/STLFunctionalExtras.h
 * （不是 FunctionRef.h），且只有一个按参数包展开的偏特化。
 * Frama-Clang 0.0.20 实例化不了参数包形式，故按 arity 展开（0-5 参）；
 * 官方构造是 `Callable &&` + 两个 enable_if 形参，模型按值接收并省掉 SFINAE。 */
namespace llvm {

template <typename Fn> class function_ref;

template <typename Ret> class function_ref<Ret()> {
public:
  int reserved;
  function_ref() {}
  template <typename Callable> function_ref(Callable callable) {}
  Ret operator()() const;
};

template <typename Ret, typename A1> class function_ref<Ret(A1)> {
public:
  int reserved;
  function_ref() {}
  template <typename Callable> function_ref(Callable callable) {}
  Ret operator()(A1 a1) const;
};

template <typename Ret, typename A1, typename A2>
class function_ref<Ret(A1, A2)> {
public:
  int reserved;
  function_ref() {}
  template <typename Callable> function_ref(Callable callable) {}
  Ret operator()(A1 a1, A2 a2) const;
};

template <typename Ret, typename A1, typename A2, typename A3>
class function_ref<Ret(A1, A2, A3)> {
public:
  int reserved;
  function_ref() {}
  template <typename Callable> function_ref(Callable callable) {}
  Ret operator()(A1 a1, A2 a2, A3 a3) const;
};

template <typename Ret, typename A1, typename A2, typename A3, typename A4>
class function_ref<Ret(A1, A2, A3, A4)> {
public:
  int reserved;
  function_ref() {}
  template <typename Callable> function_ref(Callable callable) {}
  Ret operator()(A1 a1, A2 a2, A3 a3, A4 a4) const;
};

template <typename Ret, typename A1, typename A2, typename A3, typename A4,
          typename A5>
class function_ref<Ret(A1, A2, A3, A4, A5)> {
public:
  int reserved;
  function_ref() {}
  template <typename Callable> function_ref(Callable callable) {}
  Ret operator()(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const;
};

} // namespace llvm
#endif