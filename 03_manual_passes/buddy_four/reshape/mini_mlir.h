#ifndef RESHAPE_FULL_PASS_V2_MINI_MLIR_H
#define RESHAPE_FULL_PASS_V2_MINI_MLIR_H

// V2 conversion-only declaration subset for the fixed LLVM/MLIR revision.
// Current API owners, base paths, static types and relevant class properties
// are audited; no input/output or side-effect contracts are imposed.
// Storage algorithms and standard-library adapters are explicitly partial.
// See interfaces/README.md and audit.md for current-instance boundaries.

// x86_64-conda-linux-gnu: canonical __SIZE_TYPE__ is unsigned long.
typedef unsigned long size_t;

// Standard-library declaration subset. Both native and Frama use THIS text.
// remove_reference/conditional/enable_if/void_t have their real public forms.
// Trait query definitions below are explicit compiler-intrinsic adapters for
// the Clang18 target, not claims of converting libstdc++ trait implementations.
namespace std {
template <typename T> struct char_traits;
template <typename T> class allocator;
inline namespace __cxx11 {
template <typename CharT,typename Traits = char_traits<CharT>,typename Allocator = allocator<CharT>> class basic_string;
}
using string = __cxx11::basic_string<char, char_traits<char>, allocator<char>>;
template <typename CharT,typename Traits = char_traits<CharT>> class basic_string_view;
using string_view = basic_string_view<char,char_traits<char>>;
using nullptr_t = decltype(nullptr);
template <class E> class initializer_list {
public:
  using value_type = E;
  using reference = const E&;
  using const_reference = const E&;
  using size_type = size_t;
  using iterator = const E*;
  using const_iterator = const E*;
private:
  iterator _M_array;
  size_type _M_len;
  constexpr initializer_list(const_iterator, size_type);
public:
  constexpr initializer_list() noexcept : _M_array(nullptr), _M_len(0) {}
  constexpr size_type size() const noexcept;
  constexpr const_iterator begin() const noexcept;
  constexpr const_iterator end() const noexcept;
};

template <class T, T v> struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant<T, v>;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};
using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;
template <bool V> using bool_constant = integral_constant<bool, V>;
template <typename T> struct remove_reference { using type = T; };
template <typename T> struct remove_reference<T &> { using type = T; };
template <typename T> struct remove_reference<T &&> { using type = T; };
template <typename T> using remove_reference_t = typename remove_reference<T>::type;
template <typename T> struct remove_const { using type = T; };
template <typename T> struct remove_const<const T> { using type = T; };
template <typename T> struct remove_volatile { using type = T; };
template <typename T> struct remove_volatile<volatile T> { using type = T; };
template <typename T> struct remove_cv : remove_const<typename remove_volatile<T>::type> {};
template <typename T> using remove_cv_t = typename remove_cv<T>::type;
template <bool, class T = void> struct enable_if {};
template <class T> struct enable_if<true, T> { using type = T; };
template <bool B, class T = void> using enable_if_t = typename enable_if<B,T>::type;
template <bool B, class T, class F> struct conditional { using type = T; };
template <class T, class F> struct conditional<false,T,F> { using type = F; };
template <bool B, class T, class F> using conditional_t = typename conditional<B,T,F>::type;
template <class...> using void_t = void;
template <typename T,typename... Args> struct is_constructible : bool_constant<__is_constructible(T,Args...)> {};
template <class T> struct is_class : bool_constant<__is_class(T)> {};
template <class B, class D> struct is_base_of : bool_constant<__is_base_of(B,D)> {};
template <class T, class U> struct is_same : false_type {};
template <class T> struct is_same<T,T> : true_type {};
template <class T> struct is_void : is_same<void,remove_cv_t<T>> {};
template <class T> struct is_integral : bool_constant<__is_integral(T)> {};
template <class T, class U> struct is_convertible : bool_constant<__is_convertible(T,U)> {};
template <class T, class U> struct is_assignable : bool_constant<__is_assignable(T,U)> {};
template <class T> struct is_trivially_copy_constructible : bool_constant<__is_trivially_constructible(T,const T&)> {};
template <class T> struct is_trivially_move_constructible : bool_constant<__is_trivially_constructible(T,T&&)> {};
template <class T> struct is_trivially_destructible : bool_constant<__is_trivially_destructible(T)> {};
template <class...> struct conjunction : true_type {};
template <class B> struct conjunction<B> : B {};
template <class B, class... Bs> struct conjunction<B,Bs...> : conditional_t<bool(B::value),conjunction<Bs...>,B> {};
template <class T> struct decay { using type = remove_cv_t<remove_reference_t<T>>; };
// Current consumers are closure and handle types, not array/function decay.
// Other decay inputs are outside this subset and must be extended before use.
template <class T> using decay_t = typename decay<T>::type;
template <class T> T&& declval() noexcept;
template <typename T>
constexpr typename std::remove_reference<T>::type &&move(T &&value) noexcept { return static_cast<typename std::remove_reference<T>::type&&>(value); }
template <typename T>
constexpr T&& forward(remove_reference_t<T>& value) noexcept { return static_cast<T&&>(value); }
template <typename T>
constexpr T&& forward(remove_reference_t<T>&& value) noexcept { static_assert(!__is_lvalue_reference(T), "bad forward call"); return static_cast<T&&>(value); }
}
namespace std {
template <typename...> class tuple;
template <size_t I,typename T> struct tuple_element;
template <size_t I,typename H,typename... Ts>
struct tuple_element<I,tuple<H,Ts...>> : tuple_element<I-1,tuple<Ts...>> {};
template <typename H,typename... Ts> struct tuple_element<0,tuple<H,Ts...>> { using type = H; };
template <size_t I,typename T> using tuple_element_t = typename tuple_element<I,T>::type;
template <typename B> struct negation : bool_constant<!bool(B::value)> {};
template <typename...> struct disjunction : false_type {};
template <typename B> struct disjunction<B> : B {};
template <typename B,typename... Bs>
struct disjunction<B,Bs...> : conditional_t<bool(B::value),B,disjunction<Bs...>> {};
}
// Standard ownership/container declaration adapters. Public special-member
// forms and current canonical types are retained. Allocator/tuple/vector
// storage algorithms and unused overloads are NOT part of this mini.
namespace std {
template <typename T> class __new_allocator {};
template <typename T> class allocator : public __new_allocator<T> {
public:
  allocator() noexcept;
  allocator(const allocator&) noexcept;
  template <typename U> allocator(const allocator<U>&) noexcept;
  ~allocator() noexcept;
};
template <typename T> struct default_delete {
  constexpr default_delete() noexcept = default;
  void operator()(T*) const;
};
template <typename T,typename D> class __uniq_ptr_impl {
public:
  using pointer = T*;
  __uniq_ptr_impl() = default;
  __uniq_ptr_impl(__uniq_ptr_impl&&) noexcept;
  __uniq_ptr_impl& operator=(__uniq_ptr_impl&&) noexcept;
  // The real tuple<pointer,D> storage is opaque here; no layout claim.
};
template <typename T,typename D,bool = __is_constructible(D,D&&),
          bool = __is_assignable(D&,D&&)>
struct __uniq_ptr_data : __uniq_ptr_impl<T,D> {
  using __uniq_ptr_impl<T,D>::__uniq_ptr_impl;
  __uniq_ptr_data(__uniq_ptr_data&&) = default;
  __uniq_ptr_data& operator=(__uniq_ptr_data&&) = default;
};
template <typename T,typename D = default_delete<T>> class unique_ptr {
  __uniq_ptr_data<T,D> _M_t;
public:
  using pointer = typename __uniq_ptr_impl<T,D>::pointer;
  using element_type = T;
  using deleter_type = D;
  template <typename Del = D,typename = enable_if_t<!__is_pointer(Del) && __is_constructible(Del)>>
  constexpr unique_ptr() noexcept;
  unique_ptr(unique_ptr&&) = default;
  unique_ptr& operator=(unique_ptr&&) = default;
  ~unique_ptr() noexcept;
  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;
};
template <typename T,typename Alloc> struct _Vector_base {
  _Vector_base() = default;
  _Vector_base(_Vector_base&&) = default;
  ~_Vector_base() noexcept;
  // Real allocator and three-pointer storage is outside current layout use.
};
template <typename T,typename Alloc = allocator<T>>
class vector : protected _Vector_base<T,Alloc> {
public:
  vector() = default;
  vector(const vector&);
  vector(vector&&) noexcept = default;
  vector& operator=(const vector&);
  vector& operator=(vector&&) noexcept;
  ~vector() noexcept;
};
}
namespace __gnu_cxx { enum _Lock_policy { _S_single, _S_mutex, _S_atomic }; }
namespace std {
template <typename T> struct is_array : bool_constant<__is_array(T)> {};
template <typename T> struct remove_extent { using type = T; };
template <typename T> struct remove_extent<T[]> { using type = T; };
template <typename T,size_t N> struct remove_extent<T[N]> { using type = T; };
template <__gnu_cxx::_Lock_policy L> class _Sp_counted_base;
template <__gnu_cxx::_Lock_policy L = __gnu_cxx::_S_atomic> class __shared_count {
public:
  constexpr __shared_count() noexcept : _M_pi(0) {}
  __shared_count(const __shared_count&) noexcept;
  __shared_count& operator=(const __shared_count&) noexcept;
  ~__shared_count() noexcept;
private:
  _Sp_counted_base<L>* _M_pi;
};
template <typename T,__gnu_cxx::_Lock_policy L,
          bool = is_array<T>::value,bool = is_void<T>::value>
class __shared_ptr_access {};
template <typename T,__gnu_cxx::_Lock_policy L = __gnu_cxx::_S_atomic>
class __shared_ptr : public __shared_ptr_access<T,L> {
public:
  using element_type = typename remove_extent<T>::type;
  constexpr __shared_ptr() noexcept;
  __shared_ptr(const __shared_ptr&) noexcept = default;
  __shared_ptr(__shared_ptr&&) noexcept;
  __shared_ptr& operator=(const __shared_ptr&) noexcept = default;
  __shared_ptr& operator=(__shared_ptr&&) noexcept;
  ~__shared_ptr() = default;
private:
  element_type* _M_ptr;
  __shared_count<L> _M_refcount;
};
template <typename T> class shared_ptr : public __shared_ptr<T> {
public:
  using element_type = typename __shared_ptr<T>::element_type;
  constexpr shared_ptr() noexcept;
  shared_ptr(const shared_ptr&) noexcept = default;
  shared_ptr(shared_ptr&&) noexcept;
  shared_ptr& operator=(const shared_ptr&) noexcept = default;
  shared_ptr& operator=(shared_ptr&&) noexcept;
};
}

namespace mlir { class Type; class Operation; }
// Equivalent C++ assert expansion from the configured sysroot assert.h.
extern "C" void __assert_fail(const char*,const char*,unsigned int,const char*) noexcept __attribute__((noreturn));
#define MINI_LLVM_ASSERT(expr) (static_cast<bool>(expr) ? void(0) : __assert_fail(#expr,__FILE__,__LINE__,__PRETTY_FUNCTION__))
namespace std {
template <typename T,typename U> struct pair;
template <typename T = void> struct less;
template <typename Signature> class function;
template <typename Key,typename T,typename Compare = less<Key>,
          typename Alloc = allocator<pair<const Key,T>>>
class map {
public:
  map() = default;
  map(const map&) = default;
  map(map&&) = default;
  map& operator=(const map&) = default;
  map& operator=(map&&) = default;
  ~map();
  // Red-black tree storage is not modeled; no map operation is invoked here.
};
}

namespace llvm {
// STLExtras.h function_traits used by the REAL walk constraint.
template <typename T,bool isClass = std::is_class<T>::value>
struct function_traits : public function_traits<decltype(&T::operator())> {};
template <typename C,typename Ret,typename... Args>
struct function_traits<Ret(C::*)(Args...) const,false> {
  enum { num_args = sizeof...(Args) };
  using result_t = Ret;
  template <size_t I> using arg_t = std::tuple_element_t<I,std::tuple<Args...>>;
};
template <typename C,typename Ret,typename... Args>
struct function_traits<Ret(C::*)(Args...),false> : public function_traits<Ret(C::*)(Args...) const> {};
template <typename Ret,typename... Args>
struct function_traits<Ret(*)(Args...),false> {
  enum { num_args = sizeof...(Args) };
  using result_t = Ret;
  template <size_t I> using arg_t = std::tuple_element_t<I,std::tuple<Args...>>;
};
template <typename Ret,typename... Args>
struct function_traits<Ret(*const)(Args...),false> : public function_traits<Ret(*)(Args...)> {};
template <typename Ret,typename... Args>
struct function_traits<Ret(&)(Args...),false> : public function_traits<Ret(*)(Args...)> {};
template <typename T,typename... Ts>
using is_one_of = std::disjunction<std::is_same<T,Ts>...>;
template <typename T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
// STLFunctionalExtras.h:37-82. Real type erasure wrapper, no callback count stub.
template <typename Fn> class function_ref;
template <typename Ret,typename... Params>
// Current implementation adaptation below supports the verified Ret=void route.
class [[gsl::Pointer]] function_ref<Ret(Params...)> {
  Ret (*callback)(long callable,Params... params) = nullptr;
  long callable;
  template <typename Callable>
  static Ret callback_fn(long callable,Params... params) {
    (*reinterpret_cast<Callable*>(callable))(std::forward<Params>(params)...); return;
  }
public:
  function_ref() = default;
  function_ref(std::nullptr_t) {}
  template <typename Callable>
  function_ref(Callable&& callable [[clang::lifetimebound]],
    std::enable_if_t<!std::is_same<remove_cvref_t<Callable>,function_ref>::value>* = nullptr,
    std::enable_if_t<std::is_void<Ret>::value || std::is_convertible<
      decltype(std::declval<Callable>()(std::declval<Params>()...)),Ret>::value>* = nullptr)
    : callback(callback_fn<std::remove_reference_t<Callable>>),
      callable(reinterpret_cast<long>(&callable)) {}
  Ret operator()(Params... params) const { return callback(callable,std::forward<Params>(params)...); }
  explicit operator bool() const { return callback; }
  bool operator==(const function_ref& Other) const { return callable == Other.callable; }
};
namespace detail {
template <class,template<class...> class Op,class... Args> struct detector { using value_t = std::false_type; };
template <template<class...> class Op,class... Args>
struct detector<std::void_t<Op<Args...>>,Op,Args...> { using value_t = std::true_type; };
}
template <template<class...> class Op,class... Args>
using is_detected = typename detail::detector<void,Op,Args...>::value_t;
// PointerIntPair.h:27-94: actual declaration/storage subset; no bit operations.
template <typename T> struct PointerLikeTypeTraits;
template <typename PointerT,unsigned IntBits,typename PtrTraits> struct PointerIntPairInfo;
namespace detail {
template <typename Ptr> struct PunnedPointer {
  static_assert(sizeof(Ptr) == sizeof(long), "");
  static_assert(std::is_trivially_destructible<Ptr>::value, "");
  static_assert(std::is_trivially_copy_constructible<Ptr>::value, "");
  static_assert(std::is_trivially_move_constructible<Ptr>::value, "");
  explicit constexpr PunnedPointer(long i = 0);
private:
  alignas(Ptr) unsigned char Data[sizeof(Ptr)];
};
}
template <typename PointerTy,unsigned IntBits,typename IntType = unsigned,
          typename PtrTraits = PointerLikeTypeTraits<PointerTy>,
          typename Info = PointerIntPairInfo<PointerTy,IntBits,PtrTraits>>
class PointerIntPair {
  using InfoTy = Info;
  detail::PunnedPointer<PointerTy> Value;
public:
  constexpr PointerIntPair() = default;
  PointerIntPair(PointerTy PtrVal,IntType IntVal);
  explicit PointerIntPair(PointerTy PtrVal);
};
// Casting.h public overloads and the two current MLIR CastInfo families.
// Cast algorithms remain declarations. _v spellings are expanded to trait::value.
template <typename To,typename From,typename Enable = void> struct CastInfo;
template <typename To,typename From,typename Enable = void> struct CastIsPossible;
template <typename To> struct NullableValueCastFailed { static To castFailed(); };
template <typename To,typename From,typename Derived>
struct DefaultDoCastIfPossible { static To doCastIfPossible(From f); };
namespace detail {
template <typename OptionalDerived,typename Default>
using SelfType = std::conditional_t<std::is_same<OptionalDerived,void>::value,Default,OptionalDerived>;
template <typename T> inline bool isPresent(const T &t);
}
template <typename To,typename From,typename Derived = void>
struct ValueFromPointerCast : public CastIsPossible<To,From*>,
  public NullableValueCastFailed<To>,
  public DefaultDoCastIfPossible<To,From*,detail::SelfType<Derived,ValueFromPointerCast<To,From>>> {
  static inline To doCast(From *f);
};
// CastIsPossible's query implementation is external to this boundary.
template <typename To,typename From,typename Enable> struct CastIsPossible {
  static bool isPossible(const From&);
};
template <typename To,typename From>
struct CastInfo<To,From,std::enable_if_t<
  std::is_same<::mlir::Type,typename std::remove_const<From>::type>::value ||
  std::is_base_of<::mlir::Type,From>::value>>
  : NullableValueCastFailed<To>,DefaultDoCastIfPossible<To,From,CastInfo<To,From>> {
  static inline bool isPossible(::mlir::Type ty);
  static inline To doCast(::mlir::Type ty);
};
template <typename T> struct CastInfo<T,::mlir::Operation*>
 : public ValueFromPointerCast<T,::mlir::Operation,CastInfo<T,::mlir::Operation*>> {
  static bool isPossible(::mlir::Operation *op);
};
template <typename To,typename From>
[[nodiscard]] inline auto dyn_cast(const From &Val) -> decltype(CastInfo<To,const From>::doCastIfPossible(Val)) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) && "dyn_cast on a non-existent value");
  return CastInfo<To,const From>::doCastIfPossible(Val);
}
template <typename To,typename From>
[[nodiscard]] inline auto dyn_cast(From &Val) -> decltype(CastInfo<To,From>::doCastIfPossible(Val)) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) && "dyn_cast on a non-existent value");
  return CastInfo<To,From>::doCastIfPossible(Val);
}
template <typename To,typename From>
[[nodiscard]] inline decltype(auto) dyn_cast(From *Val) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) && "dyn_cast on a non-existent value");
  return CastInfo<To,From*>::doCastIfPossible(Val);
}
template <typename To,typename From>
[[nodiscard]] inline decltype(auto) dyn_cast(std::unique_ptr<From> &Val) {
  MINI_LLVM_ASSERT(detail::isPresent(Val) && "dyn_cast on a non-existent value");
  return CastInfo<To,std::unique_ptr<From>>::doCastIfPossible(Val);
}
// Actual generated build flag from llvm/Config/abi-breaking.h.
#define LLVM_ENABLE_ABI_BREAKING_CHECKS 1


template <bool EnableSentinelTracking, class ParentTy> class ilist_node_base;
template <bool EnableSentinelTracking, class ParentTy> class ilist_base;

/// Option to choose whether to track sentinels.
///
/// This option affects the ABI for the nodes.  When not specified explicitly,
/// the ABI depends on LLVM_ENABLE_ABI_BREAKING_CHECKS.  Specify explicitly to
/// enable \a ilist_node::isSentinel().
template <bool EnableSentinelTracking> struct ilist_sentinel_tracking {};

/// Option to specify a tag for the node type.
///
/// This option allows a single value type to be inserted in multiple lists
/// simultaneously.  See \a ilist_node for usage examples.
template <class Tag> struct ilist_tag {};

/// Option to add extra bits to the ilist_iterator.
///
/// Some use-cases (debug-info) need to know whether a position is intended
/// to be half-open or fully open, i.e. whether to include any immediately
/// adjacent debug-info in an operation. This option adds two bits to the
/// iterator class to store that information.
template <bool ExtraIteratorBits> struct ilist_iterator_bits {};

/// Option to add a pointer to this list's owner in every node.
///
/// This option causes the \a ilist_base_node for this list to contain a pointer
/// ParentTy *Parent, returned by \a ilist_base_node::getNodeBaseParent() and
/// set by \a ilist_base_node::setNodeBaseParent(ParentTy *Parent). The parent
/// value is not set automatically; the ilist owner should set itself as the
/// parent of the list sentinel, and the parent should be set on each node
/// inserted into the list. This value is also not used by
/// \a ilist_node_with_parent::getNodeParent(), but is used by \a
/// ilist_iterator::getNodeParent(), which allows the parent to be fetched from
/// any valid (non-null) iterator to this list, including the sentinel.
template <class ParentTy> struct ilist_parent {};

namespace ilist_detail {

/// Helper trait for recording whether an option is specified explicitly.
template <bool IsExplicit> struct explicitness {
  static const bool is_explicit = IsExplicit;
};
using is_explicit = explicitness<true>;
using is_implicit = explicitness<false>;

/// Check whether an option is valid.
///
/// The steps for adding and enabling a new ilist option include:
/// \li define the option, ilist_foo<Bar>, above;
/// \li add new parameters for Bar to \a ilist_detail::node_options;
/// \li add an extraction meta-function, ilist_detail::extract_foo;
/// \li call extract_foo from \a ilist_detail::compute_node_options and pass it
/// into \a ilist_detail::node_options; and
/// \li specialize \c is_valid_option<ilist_foo<Bar>> to inherit from \c
/// std::true_type to get static assertions passing in \a simple_ilist and \a
/// ilist_node.
template <class Option> struct is_valid_option : std::false_type {};

/// Extract sentinel tracking option.
///
/// Look through \p Options for the \a ilist_sentinel_tracking option, with the
/// default depending on LLVM_ENABLE_ABI_BREAKING_CHECKS.
template <class... Options> struct extract_sentinel_tracking;
template <bool EnableSentinelTracking, class... Options>
struct extract_sentinel_tracking<
    ilist_sentinel_tracking<EnableSentinelTracking>, Options...>
    : std::bool_constant<EnableSentinelTracking>, is_explicit {};
template <class Option1, class... Options>
struct extract_sentinel_tracking<Option1, Options...>
    : extract_sentinel_tracking<Options...> {};
#if LLVM_ENABLE_ABI_BREAKING_CHECKS
template <> struct extract_sentinel_tracking<> : std::true_type, is_implicit {};
#else
template <>
struct extract_sentinel_tracking<> : std::false_type, is_implicit {};
#endif
template <bool EnableSentinelTracking>
struct is_valid_option<ilist_sentinel_tracking<EnableSentinelTracking>>
    : std::true_type {};

/// Extract custom tag option.
///
/// Look through \p Options for the \a ilist_tag option, pulling out the
/// custom tag type, using void as a default.
template <class... Options> struct extract_tag;
template <class Tag, class... Options>
struct extract_tag<ilist_tag<Tag>, Options...> {
  using type = Tag;
};
template <class Option1, class... Options>
struct extract_tag<Option1, Options...> : extract_tag<Options...> {};
template <> struct extract_tag<> {
  using type = void;
};
template <class Tag> struct is_valid_option<ilist_tag<Tag>> : std::true_type {};

/// Extract iterator bits option.
///
/// Look through \p Options for the \a ilist_iterator_bits option. Defaults
/// to false.
template <class... Options> struct extract_iterator_bits;
template <bool IteratorBits, class... Options>
struct extract_iterator_bits<ilist_iterator_bits<IteratorBits>, Options...>
    : std::bool_constant<IteratorBits> {};
template <class Option1, class... Options>
struct extract_iterator_bits<Option1, Options...>
    : extract_iterator_bits<Options...> {};
template <> struct extract_iterator_bits<> : std::false_type, is_implicit {};
template <bool IteratorBits>
struct is_valid_option<ilist_iterator_bits<IteratorBits>> : std::true_type {};

/// Extract node parent option.
///
/// Look through \p Options for the \a ilist_parent option, pulling out the
/// custom parent type, using void as a default.
template <class... Options> struct extract_parent;
template <class ParentTy, class... Options>
struct extract_parent<ilist_parent<ParentTy>, Options...> {
  using type = ParentTy;
};
template <class Option1, class... Options>
struct extract_parent<Option1, Options...> : extract_parent<Options...> {};
template <> struct extract_parent<> {
  using type = void;
};
template <class ParentTy>
struct is_valid_option<ilist_parent<ParentTy>> : std::true_type {};

/// Check whether options are valid.
///
/// The conjunction of \a is_valid_option on each individual option.
template <class... Options>
struct check_options : std::conjunction<is_valid_option<Options>...> {};

/// Traits for options for \a ilist_node.
///
/// This is usually computed via \a compute_node_options.
template <class T, bool EnableSentinelTracking, bool IsSentinelTrackingExplicit,
          class TagT, bool HasIteratorBits, class ParentTy>
struct node_options {
  using value_type = T;
  using pointer = T *;
  using reference = T &;
  using const_pointer = const T *;
  using const_reference = const T &;

  static const bool enable_sentinel_tracking = EnableSentinelTracking;
  static const bool is_sentinel_tracking_explicit = IsSentinelTrackingExplicit;
  static const bool has_iterator_bits = HasIteratorBits;
  using tag = TagT;
  using parent_ty = ParentTy;
  using node_base_type = ilist_node_base<enable_sentinel_tracking, parent_ty>;
  using list_base_type = ilist_base<enable_sentinel_tracking, parent_ty>;
};

template <class T, class... Options> struct compute_node_options {
  using type = node_options<T, extract_sentinel_tracking<Options...>::value,
                            extract_sentinel_tracking<Options...>::is_explicit,
                            typename extract_tag<Options...>::type,
                            extract_iterator_bits<Options...>::value,
                            typename extract_parent<Options...>::type>;
};

} // end namespace ilist_detail
namespace ilist_detail {
template <class NodeBase,bool Enable> class node_base_prevnext;
template <class NodeBase> class node_base_prevnext<NodeBase,false> {
  NodeBase *Prev = nullptr;
  NodeBase *Next = nullptr;
};
template <class NodeBase> class node_base_prevnext<NodeBase,true> {
  PointerIntPair<NodeBase*,1> PrevAndSentinel;
  NodeBase *Next = nullptr;
};
template <class ParentTy> class node_base_parent { ParentTy *Parent = nullptr; };
template <> class node_base_parent<void> {};
template <class NodeTy,class ParentTy> class node_parent_access {};
template <class NodeTy> class node_parent_access<NodeTy,void> {};
}
template <bool EnableSentinelTracking,class ParentTy>
class ilist_node_base : public ilist_detail::node_base_prevnext<
  ilist_node_base<EnableSentinelTracking,ParentTy>,EnableSentinelTracking>,
  public ilist_detail::node_base_parent<ParentTy> {};
template <class OptionsT>
class ilist_node_impl : OptionsT::node_base_type,
 public ilist_detail::node_parent_access<ilist_node_impl<OptionsT>,typename OptionsT::parent_ty> {
protected:
  ilist_node_impl() = default;
};
template <class T,class... Options>
class ilist_node : public ilist_node_impl<typename ilist_detail::compute_node_options<T,Options...>::type> {
  static_assert(ilist_detail::check_options<Options...>::value,"Unrecognized node option!");
};
template <typename NodeTy,typename ParentTy,class... Options>
class ilist_node_with_parent : public ilist_node<NodeTy,Options...> {
protected:
  ilist_node_with_parent() = default;
};
namespace trailing_objects_internal {
// Equivalent max-alignment trait adapter avoids the uninstantiated global
// variable-template initializer; it computes from types, never a fixed 8.
template <typename... Ts> struct MaxAlignmentDecl;
template <typename T> struct MaxAlignmentDecl<T> { static constexpr size_t value = alignof(T); };
template <typename T,typename U,typename... Ts> struct MaxAlignmentDecl<T,U,Ts...> {
  static constexpr size_t tail = MaxAlignmentDecl<U,Ts...>::value;
  static constexpr size_t value = alignof(T) > tail ? alignof(T) : tail;
};
class TrailingObjectsBase {
protected:
  template <typename T> struct OverloadToken {};
};
template <int Align,typename BaseTy,typename TopTrailingObj,typename PrevTy,typename... MoreTys>
class TrailingObjectsImpl {};
template <int Align,typename BaseTy,typename TopTrailingObj,typename PrevTy,typename NextTy,typename... MoreTys>
class TrailingObjectsImpl<Align,BaseTy,TopTrailingObj,PrevTy,NextTy,MoreTys...>
 : public TrailingObjectsImpl<Align,BaseTy,TopTrailingObj,NextTy,MoreTys...> {};
template <int Align,typename BaseTy,typename TopTrailingObj,typename PrevTy>
class alignas(Align) TrailingObjectsImpl<Align,BaseTy,TopTrailingObj,PrevTy> : public TrailingObjectsBase {};
}
template <typename BaseTy,typename... TrailingTys>
class TrailingObjects : private trailing_objects_internal::TrailingObjectsImpl<
 trailing_objects_internal::MaxAlignmentDecl<TrailingTys...>::value,BaseTy,
 TrailingObjects<BaseTy,TrailingTys...>,BaseTy,TrailingTys...> {
public:
  TrailingObjects() = default;
  TrailingObjects(const TrailingObjects&) = delete;
  TrailingObjects(TrailingObjects&&) = delete;
  TrailingObjects& operator=(const TrailingObjects&) = delete;
  TrailingObjects& operator=(TrailingObjects&&) = delete;
};
template <typename T> struct Repeated;
template <typename IteratorT> class iterator_range;
// Real PointerUnion class-level validity check from STLExtras.h.
template <typename... Ts> struct TypesAreDistinct;
template <> struct TypesAreDistinct<> : std::true_type {};
template <typename T,typename... Us>
struct TypesAreDistinct<T,Us...> : std::conjunction<std::negation<is_one_of<T,Us...>>,TypesAreDistinct<Us...>> {};
namespace pointer_union_detail {
template <typename Derived,int Idx,typename... Types> class PointerUnionMembers;
template <typename Derived,int Idx> class PointerUnionMembers<Derived,Idx> {
protected:
  detail::PunnedPointer<void*> Val;
  PointerUnionMembers();
};
template <typename Derived,int Idx,typename Type,typename... Types>
class PointerUnionMembers<Derived,Idx,Type,Types...>
 : public PointerUnionMembers<Derived,Idx+1,Types...> {
  using Base = PointerUnionMembers<Derived,Idx+1,Types...>;
public:
  using Base::Base;
  PointerUnionMembers() = default;
  PointerUnionMembers(Type V);
  using Base::operator=;
  Derived& operator=(Type V);
};
}
template <typename... PTs>
class PointerUnion : public pointer_union_detail::PointerUnionMembers<PointerUnion<PTs...>,0,PTs...> {
  static_assert(sizeof...(PTs) > 0,"PointerUnion must have at least one type");
  static_assert(TypesAreDistinct<PTs...>::value,"PointerUnion alternative types cannot be repeated");
  using Base = pointer_union_detail::PointerUnionMembers<PointerUnion<PTs...>,0,PTs...>;
public:
  PointerUnion() = default;
  PointerUnion(std::nullptr_t);
  using Base::Base;
  using Base::operator=;
  const PointerUnion& operator=(std::nullptr_t);
  // Tag-table computations are not used at this declaration boundary.
};
namespace detail {
template <typename DerivedT,typename BaseT,typename T,
          typename PointerT = T*,typename ReferenceT = T&>
class indexed_accessor_range_base {
public:
  using RangeBaseT = indexed_accessor_range_base;
  class iterator;
  inline indexed_accessor_range_base(iterator begin,iterator end);
  inline indexed_accessor_range_base(const iterator_range<iterator>& range);
  inline indexed_accessor_range_base(BaseT base,long count);
protected:
  indexed_accessor_range_base(const indexed_accessor_range_base&) = default;
  indexed_accessor_range_base(indexed_accessor_range_base&&) = default;
  indexed_accessor_range_base& operator=(const indexed_accessor_range_base&) = default;
  BaseT base;
  long count;
};
}
// Declaration extraction: llvm/ADT/SmallVector.h:54-112,125-612.
// The non-trivial-element branch is declared, not used by this pass.
template <class Size_T> class SmallVectorBase {
protected:
  void *BeginX;
  Size_T Size = 0, Capacity;
  SmallVectorBase() = delete;
  inline SmallVectorBase(void *FirstEl, size_t TotalCapacity);
public:
  inline size_t size() const;
  inline size_t capacity() const;
  inline bool empty() const;
};
template <class T>
using SmallVectorSizeType = std::conditional_t<sizeof(T) < 4 && sizeof(void*) >= 8,
                                              unsigned long, unsigned int>;
template <typename T, typename = void>
class SmallVectorTemplateCommon : public SmallVectorBase<SmallVectorSizeType<T>> {
  using Base = SmallVectorBase<SmallVectorSizeType<T>>;
protected:
  inline SmallVectorTemplateCommon(size_t SizeArg);
public:
  using size_type = size_t;
  using difference_type = long;
  using value_type = T;
  using iterator = T*;
  using const_iterator = const T*;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using Base::size;
  using Base::capacity;
  using Base::empty;
  inline pointer data();
  inline const_pointer data() const;
};
template <typename T, bool = std::is_trivially_copy_constructible<T>::value &&
                             std::is_trivially_move_constructible<T>::value &&
                             std::is_trivially_destructible<T>::value>
class SmallVectorTemplateBase;
template <typename T>
class SmallVectorTemplateBase<T,true> : public SmallVectorTemplateCommon<T> {
  friend class SmallVectorTemplateCommon<T>;
protected:
  static constexpr bool TakesParamByValue = sizeof(T) <= 2 * sizeof(void*);
  using ValueParamT = std::conditional_t<TakesParamByValue,T,const T&>;
  inline SmallVectorTemplateBase(size_t SizeArg);
public:
  inline void push_back(ValueParamT Elt);
};
template <typename T> class SmallVectorImpl : public SmallVectorTemplateBase<T> {
  using SuperClass = SmallVectorTemplateBase<T>;
public:
  using iterator = typename SuperClass::iterator;
  using const_iterator = typename SuperClass::const_iterator;
  using reference = typename SuperClass::reference;
  using size_type = typename SuperClass::size_type;
protected:
  using SmallVectorTemplateBase<T>::TakesParamByValue;
  using ValueParamT = typename SuperClass::ValueParamT;
  inline explicit SmallVectorImpl(unsigned N);
  inline ~SmallVectorImpl();
public:
  SmallVectorImpl(const SmallVectorImpl&) = delete;
  SmallVectorImpl& operator=(const SmallVectorImpl& RHS);
  SmallVectorImpl& operator=(SmallVectorImpl&& RHS);
};
// Real storage, default-N calculation and current special-member declarations.
template <typename T, unsigned N> struct SmallVectorStorage {
  alignas(T) char InlineElts[N * sizeof(T)];
};
template <typename T> struct alignas(T) SmallVectorStorage<T,0> {};
template <typename T, unsigned N> class SmallVector;
template <typename T> struct CalculateSmallVectorDefaultInlinedElements {
  static constexpr size_t kPreferredSmallVectorSizeof = 64;
  static_assert(sizeof(T) <= 256, "explicit N required for large T");
  static constexpr size_t PreferredInlineBytes = kPreferredSmallVectorSizeof - sizeof(SmallVector<T,0>);
  static constexpr size_t NumElementsThatFit = PreferredInlineBytes / sizeof(T);
  static constexpr size_t value = NumElementsThatFit == 0 ? 1 : NumElementsThatFit;
};
template <typename T, unsigned N = CalculateSmallVectorDefaultInlinedElements<T>::value>
class SmallVector : public SmallVectorImpl<T>, SmallVectorStorage<T,N> {
public:
  inline SmallVector();
  inline ~SmallVector();
  inline explicit SmallVector(size_t SizeArg);
  inline SmallVector(size_t SizeArg, const T &Value);
  inline SmallVector(const SmallVector &RHS);
  inline SmallVector &operator=(const SmallVector &RHS);
  inline SmallVector(SmallVector &&RHS);
  inline SmallVector(SmallVectorImpl<T> &&RHS);
  inline SmallVector &operator=(SmallVector &&RHS);
  inline SmallVector &operator=(SmallVectorImpl<T> &&RHS);
};
class [[gsl::Pointer]] StringRef {
public:
  using size_type = size_t;
private:
  const char *Data = nullptr;
  size_t Length = 0;
public:
  StringRef() = default;
  StringRef(std::nullptr_t) = delete;
  constexpr StringRef(const char *Str [[clang::lifetimebound]]);
  constexpr StringRef(const char *data [[clang::lifetimebound]],size_t length);
  StringRef(const std::string &Str);
  constexpr StringRef(std::string_view Str);
};
// llvm/Support/LogicalResult.h:25-51. Algorithm bodies remain declarations.
struct [[nodiscard]] LogicalResult {
public:
  inline static LogicalResult success(bool IsSuccess = true);
  inline static LogicalResult failure(bool IsFailure = true);
  constexpr bool succeeded() const;
  constexpr bool failed() const;
private:
  inline LogicalResult(bool IsSuccess);
  bool IsSuccess;
};
inline LogicalResult success(bool IsSuccess = true);
inline LogicalResult failure(bool IsFailure = true);
// llvm/ADT/ArrayRef.h:41-125. Same candidate forms; bodies omitted.
// std::conjunction_v<X...> is written std::conjunction<X...>::value only
// to avoid E01's unsupported dependent variable-template initializer.
template <typename T> class [[gsl::Pointer]] [[nodiscard]] ArrayRef {
public:
  using value_type = T;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = const_pointer;
  using const_iterator = const_pointer;
  using size_type = size_t;
  using difference_type = long;
private:
  const T *Data = nullptr;
  size_type Length = 0;
public:
  ArrayRef() = default;
  inline ArrayRef(const T &OneElt [[clang::lifetimebound]]);
  constexpr ArrayRef(const T *data [[clang::lifetimebound]], size_t length);
  constexpr ArrayRef(const T *begin [[clang::lifetimebound]], const T *end);
  template <typename C,
    typename = std::enable_if_t<std::conjunction<
      std::is_convertible<decltype(std::declval<const C&>().data())*, const T*const*>,
      std::is_integral<decltype(std::declval<const C&>().size())>>::value, void>>
  constexpr ArrayRef(const C &V);
  template <size_t N> constexpr ArrayRef(const T (&Arr [[clang::lifetimebound]])[N]);
  constexpr ArrayRef(std::initializer_list<T> Vec [[clang::lifetimebound]]);
  const T* data() const;
  inline size_t size() const;
};

}

namespace mlir {
using llvm::ArrayRef;
using llvm::SmallVector;
using llvm::dyn_cast;
using LogicalResult = llvm::LogicalResult;
using StringRef = llvm::StringRef;
using llvm::success;
using llvm::failure;
// Visitors.h current callback route. Generic traversal implementation is external.
// Current RetT=void wrappers use call(); return; after E10. Other RetT
// implementations are not supplied. Declaration defaults/constraints stay real.
enum class WalkOrder { PreOrder, PostOrder };
struct ForwardIterator;
class Region;
class Block;
namespace detail {
template <typename Ret,typename Arg,typename... Rest>
Arg first_argument_type(Ret(*)(Arg,Rest...));
template <typename Ret,typename F,typename Arg,typename... Rest>
Arg first_argument_type(Ret(F::*)(Arg,Rest...));
template <typename Ret,typename F,typename Arg,typename... Rest>
Arg first_argument_type(Ret(F::*)(Arg,Rest...) const);
template <typename F>
decltype(first_argument_type(&F::operator())) first_argument_type(F);
template <typename T> using first_argument = decltype(first_argument_type(std::declval<T>()));
// Exact external boundary from Visitors.h:140: no traversal/count placeholder.
template <typename Iterator>
void walk(Operation *op,llvm::function_ref<void(Operation*)> callback,WalkOrder order);
template <WalkOrder Order = WalkOrder::PostOrder,typename Iterator = ForwardIterator,
          typename FuncTy,typename ArgT = detail::first_argument<FuncTy>,
          typename RetT = decltype(std::declval<FuncTy>()(std::declval<ArgT>()))>
std::enable_if_t<llvm::is_one_of<ArgT,Operation*,Region*,Block*>::value,RetT>
walk(Operation *op,FuncTy&& callback) {
  return detail::walk<Iterator>(op,llvm::function_ref<RetT(ArgT)>(callback),Order);
}
template <WalkOrder Order = WalkOrder::PostOrder,typename Iterator = ForwardIterator,
          typename FuncTy,typename ArgT = detail::first_argument<FuncTy>,
          typename RetT = decltype(std::declval<FuncTy>()(std::declval<ArgT>()))>
std::enable_if_t<!llvm::is_one_of<ArgT,Operation*,Region*,Block*>::value &&
                std::is_same<RetT,void>::value,RetT>
walk(Operation *op,FuncTy&& callback) {
  auto wrapperFn = [&](Operation *op) {
    {
      auto derivedOp = dyn_cast<ArgT>(op);
      if (derivedOp)
        callback(derivedOp);
    }
  };
  detail::walk<Iterator>(op,llvm::function_ref<RetT(Operation*)>(wrapperFn),Order); return;
}
template <typename FnT>
using walkResultType = decltype(walk(nullptr,std::declval<FnT>()));
}


class MLIRContext;
// Operation remains forward-declared here; full declaration follows operands.
class PatternRewriter;
namespace OpTrait {
template <typename ConcreteType, template <typename> class TraitType>
class TraitBase {
protected:
  inline Operation *getOperation();
};
}
namespace detail {
// InterfaceSupport.h:68-140. Concept/model declarations stay opaque.
template <typename ConcreteType, typename ValueT, typename Traits,
          typename BaseType,
          template <typename, template <typename> class> class BaseTrait>
class Interface : public BaseType {
public:
  using Concept = typename Traits::Concept;
  template <typename T> using Model = typename Traits::template Model<T>;
  template <typename T> using FallbackModel = typename Traits::template FallbackModel<T>;
  template <typename T, typename U> using ExternalModel = typename Traits::template ExternalModel<T,U>;
  using InterfaceBase = Interface<ConcreteType,ValueT,Traits,BaseType,BaseTrait>;
  using ValueType = ValueT;
  template <typename ConcreteT> struct Trait : public BaseTrait<ConcreteT,Trait> {
    using ModelT = Model<ConcreteT>;
  };
  explicit Interface(ValueT t = ValueT());
  Interface(std::nullptr_t);
  template <typename T, std::enable_if_t<std::is_base_of<Trait<T>,T>::value>* = nullptr>
  Interface(T t);
  Interface(ValueT t, const Concept *conceptImpl);
  Interface(ValueT t, std::nullptr_t);
private:
  Concept *conceptImpl;
};
}

class TypeStorage;
class TypeID;
namespace detail {
class TypeUniquer;
template <typename ConcreteT,typename BaseT,typename StorageT,typename UniquerT,
          template<typename> class... Traits> class StorageUserBase;
}
class Type {
public:
  template <typename ConcreteType,typename BaseType,typename StorageType,
            template<typename> class... Traits>
  using TypeBase = detail::StorageUserBase<ConcreteType,BaseType,StorageType,
                                          detail::TypeUniquer,Traits...>;
  using ImplType = TypeStorage;
  constexpr Type() = default;
  inline Type(const ImplType *impl);
  Type(const Type &other) = default;
  Type &operator=(const Type &other) = default;
  inline explicit operator bool() const;
  inline bool operator!() const;
  inline bool operator!=(Type other) const;
protected:
  ImplType *impl{nullptr};
};
// Types.h:249-280 and StorageUniquerSupport.h:42-55.
namespace detail {
template <typename ConcreteType,template<typename> class TraitType>
class StorageUserTraitBase {
protected:
  ConcreteType getInstance() const;
};
}
namespace TypeTrait {
template <typename ConcreteType,template<typename> class TraitType>
using TraitBase = detail::StorageUserTraitBase<ConcreteType,TraitType>;
}
template <typename ConcreteType,typename Traits>
class TypeInterface : public detail::Interface<ConcreteType,Type,Traits,Type,TypeTrait::TraitBase> {
public:
  using Base = TypeInterface<ConcreteType,Traits>;
  using InterfaceBase = detail::Interface<ConcreteType,Type,Traits,Type,TypeTrait::TraitBase>;
  using InterfaceBase::InterfaceBase;
};
// Real current Type/TypeInterface/StorageUserBase paths follow.
namespace detail {
struct ShapedTypeInterfaceTraits {
  struct Concept;
  template <typename T> class Model;
  template <typename T> class FallbackModel;
  template <typename T,typename U> class ExternalModel;
};
template <typename ConcreteType> struct ShapedTypeTrait;
}
class ShapedType : public TypeInterface<ShapedType,detail::ShapedTypeInterfaceTraits> {
public:
  using TypeInterface<ShapedType,detail::ShapedTypeInterfaceTraits>::TypeInterface;
  template <typename ConcreteType> struct Trait : public detail::ShapedTypeTrait<ConcreteType> {};
};
namespace detail {
template <typename ConcreteType>
struct ShapedTypeTrait : public TypeInterface<ShapedType,detail::ShapedTypeInterfaceTraits>::template Trait<ConcreteType> {
  inline size_t getNumDynamicDims() const;
};
}
class TensorType;
template struct detail::ShapedTypeTrait<TensorType>;
// BuiltinTypes.h:55-93: actual additional mixin and inherited constructors.
class TensorType : public Type, public ShapedType::Trait<TensorType> {
public:
  using Type::Type;
  inline operator ShapedType() const;
};

// StorageUniquerSupport.h:92-107; bodies of uniquing/verification are omitted.
template <typename ConcreteType>
class ValueSemantics : public TypeTrait::TraitBase<ConcreteType,ValueSemantics> {};
namespace detail {
struct RankedTensorTypeStorage;
template <typename ConcreteT,typename BaseT,typename StorageT,typename UniquerT,
          template<typename> class... Traits>
class StorageUserBase : public BaseT, public Traits<ConcreteT>... {
public:
  using BaseT::BaseT;
  using Base = StorageUserBase<ConcreteT,BaseT,StorageT,UniquerT,Traits...>;
  using ImplType = StorageT;
  using HasTraitFn = bool (*)(TypeID);
};
}
// BuiltinTypes.h.inc:1279: exact TypeBase arguments and actual member owner.
class RankedTensorType : public Type::TypeBase<RankedTensorType,TensorType,
  detail::RankedTensorTypeStorage,ShapedType::Trait,ValueSemantics> {
public:
  using Base::Base;
  using ShapedType::Trait<RankedTensorType>::getNumDynamicDims;
};

namespace detail { class ValueImpl; }
class Value {
public:
  constexpr Value(detail::ValueImpl *impl = nullptr) : impl(impl) {}
  template <typename OpTy> inline OpTy getDefiningOp() const;
protected:
  detail::ValueImpl *impl;
};
class OpOperand;
class BlockArgument;
class OperandRange;
class ResultRange;
namespace detail { class OpResultImpl; }
class ValueRange final : public llvm::detail::indexed_accessor_range_base<
  ValueRange,llvm::PointerUnion<const Value*,OpOperand*,detail::OpResultImpl*,
    const llvm::Repeated<Value>*>,Value,Value,Value> {
public:
  using OwnerT = llvm::PointerUnion<const Value*,OpOperand*,detail::OpResultImpl*,
                                    const llvm::Repeated<Value>*>;
  using RangeBaseT::RangeBaseT;
  template <typename Arg,typename = std::enable_if_t<
    std::is_constructible<ArrayRef<Value>,Arg>::value && !std::is_convertible<Arg,Value>::value>>
  inline ValueRange(Arg&& arg [[clang::lifetimebound]]);
  inline ValueRange(const Value &value [[clang::lifetimebound]]);
  inline ValueRange(const std::initializer_list<Value>& values [[clang::lifetimebound]]);
  inline ValueRange(ArrayRef<BlockArgument> values);
  ValueRange(ArrayRef<Value> values = {});
  ValueRange(OperandRange values);
  ValueRange(ResultRange values);
  inline ValueRange(const llvm::Repeated<Value>& repeatedValue [[clang::lifetimebound]]);
};
namespace detail {
template <typename Ty> struct TypedValue : Value {
  using Value::Value;
  using ValueType = Ty;
  inline static bool classof(Value value);
  template <typename ToTy,typename = typename std::enable_if<
    std::is_assignable<typename ToTy::ValueType&,Ty>::value>::type>
  inline operator ToTy() const;
  inline Ty getType() const;
  inline void setType(Ty ty);
};
}
template <typename Ty,typename Value = mlir::Value>
using TypedValue = std::conditional_t<std::is_same<Ty,mlir::Type>::value,
                                      mlir::Value,detail::TypedValue<Ty>>;
namespace detail {
class IROperandBase {
protected:
  inline IROperandBase(Operation *owner);
  inline IROperandBase(IROperandBase&& other);
  inline IROperandBase& operator=(IROperandBase&& other);
  IROperandBase(const IROperandBase& use) = delete;
  IROperandBase& operator=(const IROperandBase& use) = delete;
  inline ~IROperandBase();
  IROperandBase *nextUse = nullptr;
  IROperandBase **back = nullptr;
private:
  Operation *const owner;
};
}
template <typename DerivedT,typename IRValueT>
class IROperand : public detail::IROperandBase {
public:
  inline IROperand(Operation *owner);
  inline IROperand(Operation *owner,IRValueT value);
  inline IROperand(IROperand&& other);
  inline IROperand& operator=(IROperand&& other);
  inline void set(IRValueT newValue);
private:
  IRValueT value = {};
};
class OpOperand : public IROperand<OpOperand,Value> {
public:
  inline void assign(Value value);
private:
  friend class OperandStorage;
  using IROperand<OpOperand,Value>::IROperand;
};
class Location;
class OperationName;
class DictionaryAttr;
class PropertyRef;
namespace detail {
class alignas(8) OperandStorage {
public:
  OperandStorage(Operation *owner,OpOperand *trailingOperands,ValueRange values);
  ~OperandStorage();
  // Only alignment is needed by the real TrailingObjects base computation.
};
enum class OpProperties : char {};
}
class BlockOperand : public IROperand<BlockOperand,Block*> {
public:
  using IROperand<BlockOperand,Block*>::IROperand;
};
// Region.h:44,342: real block-list dependency, including copy deletion.
} // namespace mlir
namespace llvm {
template <bool EnableSentinelTracking,class ParentTy> class ilist_base {
public:
  using node_base_type = ilist_node_base<EnableSentinelTracking,ParentTy>;
};
template <class OptionsT,bool IsReverse,bool IsConst> class ilist_iterator;
template <class OptionsT,bool IsReverse,bool IsConst> class ilist_iterator_w_bits;
template <class OptionsT,bool IsReverse,bool IsConst>
using ilist_select_iterator_type = std::conditional_t<OptionsT::has_iterator_bits,
  ilist_iterator_w_bits<OptionsT,IsReverse,IsConst>,ilist_iterator<OptionsT,IsReverse,IsConst>>;
namespace ilist_detail {
struct NodeAccess {};
template <class OptionsT> struct SpecificNodeAccess : NodeAccess {
protected:
  using pointer = typename OptionsT::pointer;
  using const_pointer = typename OptionsT::const_pointer;
  using node_type = ilist_node_impl<OptionsT>;
  inline static node_type *getNodePtr(pointer N);
  inline static const node_type *getNodePtr(const_pointer N);
  inline static pointer getValuePtr(node_type *N);
  inline static const_pointer getValuePtr(const node_type *N);
};
}
template <class OptionsT> class ilist_sentinel : public ilist_node_impl<OptionsT> {
public:
  inline ilist_sentinel();
};
template <typename T,class... Options>
class simple_ilist
 : ilist_detail::compute_node_options<T,Options...>::type::list_base_type,
   ilist_detail::SpecificNodeAccess<typename ilist_detail::compute_node_options<T,Options...>::type> {
  static_assert(ilist_detail::check_options<Options...>::value,"Unrecognized node option!");
  using OptionsT = typename ilist_detail::compute_node_options<T,Options...>::type;
  using list_base_type = typename OptionsT::list_base_type;
  ilist_sentinel<OptionsT> Sentinel;
public:
  using value_type = typename OptionsT::value_type;
  using pointer = typename OptionsT::pointer;
  using reference = typename OptionsT::reference;
  using const_pointer = typename OptionsT::const_pointer;
  using const_reference = typename OptionsT::const_reference;
  using iterator = ilist_select_iterator_type<OptionsT,false,false>;
  using const_iterator = ilist_select_iterator_type<OptionsT,false,true>;
  using reverse_iterator = ilist_select_iterator_type<OptionsT,true,false>;
  using const_reverse_iterator = ilist_select_iterator_type<OptionsT,true,true>;
  using size_type = size_t;
  using difference_type = long; // target canonical ptrdiff_t
  simple_ilist() = default;
  ~simple_ilist() = default;
  simple_ilist(const simple_ilist&) = delete;
  simple_ilist& operator=(const simple_ilist&) = delete;
  inline simple_ilist(simple_ilist&& X);
  inline simple_ilist& operator=(simple_ilist&& X);
};
template <typename NodeTy> struct ilist_alloc_traits { inline static void deleteNode(NodeTy *V); };
template <typename NodeTy> struct ilist_callback_traits {};
template <typename NodeTy> struct ilist_node_traits : ilist_alloc_traits<NodeTy>,ilist_callback_traits<NodeTy> {};
template <typename NodeTy> struct ilist_traits : public ilist_node_traits<NodeTy> {};
template <typename Ty> struct ilist_traits<const Ty> {};
template <> struct ilist_traits<::mlir::Block> : public ilist_alloc_traits<::mlir::Block> {
  using Block = ::mlir::Block;
  using block_iterator = simple_ilist<Block>::iterator;
  void addNodeToList(Block *block);
  void removeNodeFromList(Block *block);
  void transferNodesFromList(ilist_traits<Block>& otherList,block_iterator first,block_iterator last);
private:
  mlir::Region *getParentRegion();
};
template <class IntrusiveListT,class TraitsT>
class iplist_impl : public TraitsT,IntrusiveListT {
  using base_list_type = IntrusiveListT;
public:
  iplist_impl() = default;
  iplist_impl(const iplist_impl&) = delete;
  iplist_impl& operator=(const iplist_impl&) = delete;
  inline iplist_impl(iplist_impl&& X);
  inline iplist_impl& operator=(iplist_impl&& X);
  inline ~iplist_impl();
};
template <class T,class... Options>
class iplist : public iplist_impl<simple_ilist<T,Options...>,ilist_traits<T>> {
  using iplist_impl_type = typename iplist::iplist_impl;
public:
  iplist() = default;
  iplist(const iplist& X) = delete;
  iplist& operator=(const iplist& X) = delete;
  inline iplist(iplist&& X);
  inline iplist& operator=(iplist&& X);
};
}
namespace mlir {
class Region {
public:
  Region() = default;
  explicit Region(Operation *container);
  ~Region();
  using BlockListType = llvm::iplist<Block>;
private:
  BlockListType blocks;
  Operation *container = nullptr;

};
class alignas(8) Operation final : public llvm::ilist_node_with_parent<Operation,Block>,
  private llvm::TrailingObjects<Operation,detail::OperandStorage,detail::OpProperties,
                                BlockOperand,Region,OpOperand> {
public:
  inline bool use_empty();
  template <WalkOrder Order = WalkOrder::PostOrder,typename Iterator = ForwardIterator,
            typename FnT,typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 1,RetT>
  walk(FnT&& callback) {
    detail::walk<Order,Iterator>(this,std::forward<FnT>(callback)); return;
  }
  template <typename FnT,typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 2,RetT>
  walk(FnT&& callback);


private:
  Operation(Location location,OperationName name,unsigned numResults,
            unsigned numSuccessors,unsigned numRegions,int propertiesStorageSize,
            DictionaryAttr attributes,PropertyRef properties,bool hasOperandStorage);
  ~Operation();
  // Operation's IR storage is outside pointer-only APIs and not a layout model.
};

// OpDefinition.h:98-260: actual member owner and storage.
class OpState {
public:
  inline explicit operator bool();
  inline operator Operation *() const;
  inline Operation *operator->() const;
  inline Operation *getOperation();
  inline MLIRContext *getContext();
  inline explicit OpState(Operation *state);
  template <WalkOrder Order = WalkOrder::PostOrder,typename Iterator = ForwardIterator,
            typename FnT,typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 1,RetT>
  walk(FnT&& callback) {
    state->walk<Order,Iterator>(std::forward<FnT>(callback)); return;
  }
  template <typename FnT,typename RetT = detail::walkResultType<FnT>>
  std::enable_if_t<llvm::function_traits<std::decay_t<FnT>>::num_args == 2,RetT>
  walk(FnT&& callback);

private:
  Operation *state;
};
// OpDefinition.h:1711-1765. Algorithm hooks remain outside this subset.

namespace OpTrait {
template <typename ResultType> class OneTypedResult {
public:
 template <typename ConcreteType> class Impl : public TraitBase<ConcreteType,OneTypedResult<ResultType>::template Impl> {
 public:
  inline ::mlir::detail::TypedValue<ResultType> getResult();
  inline operator ::mlir::detail::TypedValue<ResultType>();
  inline ResultType getType();
 };
}; }
namespace OpTrait {
namespace detail {
template <typename ConcreteType,template <typename> class TraitType>
struct MultiOperandTraitBase : public TraitBase<ConcreteType,TraitType> {};
}
template <unsigned N> class NOperands {
public:
 static_assert(N > 1,"use ZeroOperands/OneOperand for N < 2");
 template <typename ConcreteType> class Impl : public detail::MultiOperandTraitBase<ConcreteType,NOperands<N>::template Impl> {};
}; }
template <typename ConcreteType, template <typename T> class... Traits>
class Op : public OpState, public Traits<ConcreteType>... {
public:
  using OpState::getOperation;
  using ConcreteOpType = ConcreteType;
  inline explicit Op();
  inline Op(std::nullptr_t);
  inline explicit Op(Operation *state);
};

template <typename ConcreteType, typename Traits>
class OpInterface : public detail::Interface<ConcreteType,Operation*,Traits,
                                             Op<ConcreteType>,OpTrait::TraitBase> {
public:
  using Base = OpInterface<ConcreteType,Traits>;
  using InterfaceBase = detail::Interface<ConcreteType,Operation*,Traits,
                                          Op<ConcreteType>,OpTrait::TraitBase>;
  using InterfaceBase::InterfaceBase;
};
namespace OpTrait { template <typename ConcreteType> class ZeroRegions : public TraitBase<ConcreteType,ZeroRegions> {}; }
namespace OpTrait { template <typename ConcreteType> class OneResult : public TraitBase<ConcreteType,OneResult> {}; }

namespace OpTrait { template <typename ConcreteType> class ZeroSuccessors : public TraitBase<ConcreteType,ZeroSuccessors> {}; }
namespace OpTrait { template <typename ConcreteType> class OpInvariants : public TraitBase<ConcreteType,OpInvariants> {}; }
namespace detail { struct ConditionallySpeculatableInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct ConditionallySpeculatableTrait;
}
class ConditionallySpeculatable : public ::mlir::OpInterface<ConditionallySpeculatable,detail::ConditionallySpeculatableInterfaceTraits> {
public:
 using ::mlir::OpInterface<ConditionallySpeculatable,detail::ConditionallySpeculatableInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::ConditionallySpeculatableTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct ConditionallySpeculatableTrait : public ::mlir::OpInterface<ConditionallySpeculatable,detail::ConditionallySpeculatableInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace OpTrait { template <typename ConcreteType> struct AlwaysSpeculatableImplTrait : public TraitBase<ConcreteType,AlwaysSpeculatableImplTrait> {}; }
namespace detail { struct MemoryEffectOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct MemoryEffectOpInterfaceTrait;
}
class MemoryEffectOpInterface : public ::mlir::OpInterface<MemoryEffectOpInterface,detail::MemoryEffectOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<MemoryEffectOpInterface,detail::MemoryEffectOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::MemoryEffectOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct MemoryEffectOpInterfaceTrait : public ::mlir::OpInterface<MemoryEffectOpInterface,detail::MemoryEffectOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace detail { struct InferTypeOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct InferTypeOpInterfaceTrait;
}
class InferTypeOpInterface : public ::mlir::OpInterface<InferTypeOpInterface,detail::InferTypeOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<InferTypeOpInterface,detail::InferTypeOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::InferTypeOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct InferTypeOpInterfaceTrait : public ::mlir::OpInterface<InferTypeOpInterface,detail::InferTypeOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace detail { struct InferShapedTypeOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct InferShapedTypeOpInterfaceTrait;
}
class InferShapedTypeOpInterface : public ::mlir::OpInterface<InferShapedTypeOpInterface,detail::InferShapedTypeOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<InferShapedTypeOpInterface,detail::InferShapedTypeOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::InferShapedTypeOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct InferShapedTypeOpInterfaceTrait : public ::mlir::OpInterface<InferShapedTypeOpInterface,detail::InferShapedTypeOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace OpTrait { template <typename ConcreteType> class InferShapedTypeOpAdaptor : public TraitBase<ConcreteType,InferShapedTypeOpAdaptor> {}; }
namespace OpTrait { template <typename ConcreteType> class InferTensorType : public TraitBase<ConcreteType,InferTensorType> {}; }
namespace tosa {
namespace detail { struct TosaOpInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct TosaOpTrait;
}
class TosaOp : public ::mlir::OpInterface<TosaOp,detail::TosaOpInterfaceTraits> {
public:
 using ::mlir::OpInterface<TosaOp,detail::TosaOpInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::TosaOpTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct TosaOpTrait : public ::mlir::OpInterface<TosaOp,detail::TosaOpInterfaceTraits>::template Trait<ConcreteOp> {}; }
}
namespace tosa {
namespace detail { struct QueryProfileInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
}
class QueryProfileInterface : public ::mlir::OpInterface<QueryProfileInterface,detail::QueryProfileInterfaceTraits> {
public:
 using ::mlir::OpInterface<QueryProfileInterface,detail::QueryProfileInterfaceTraits>::OpInterface;
};
}
namespace tosa {
namespace detail { struct QueryExtensionInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
}
class QueryExtensionInterface : public ::mlir::OpInterface<QueryExtensionInterface,detail::QueryExtensionInterfaceTraits> {
public:
 using ::mlir::OpInterface<QueryExtensionInterface,detail::QueryExtensionInterfaceTraits>::OpInterface;
};
}
namespace OpTrait { namespace tosa { template <typename ConcreteType> class TosaResolvableShapeOperands : public TraitBase<ConcreteType,TosaResolvableShapeOperands> {}; } }
namespace OpTrait { template <typename ConcreteType> class OneRegion : public TraitBase<ConcreteType,OneRegion> {}; }
namespace OpTrait { template <typename ConcreteType> class ZeroResults : public TraitBase<ConcreteType,ZeroResults> {}; }
namespace OpTrait { template <typename ConcreteType> class ZeroOperands : public TraitBase<ConcreteType,ZeroOperands> {}; }
namespace detail { struct BytecodeOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct BytecodeOpInterfaceTrait;
}
class BytecodeOpInterface : public ::mlir::OpInterface<BytecodeOpInterface,detail::BytecodeOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<BytecodeOpInterface,detail::BytecodeOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::BytecodeOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct BytecodeOpInterfaceTrait : public ::mlir::OpInterface<BytecodeOpInterface,detail::BytecodeOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace OpTrait { template <typename ConcreteType> class AffineScope : public TraitBase<ConcreteType,AffineScope> {}; }
namespace OpTrait { template <typename ConcreteType> class AutomaticAllocationScope : public TraitBase<ConcreteType,AutomaticAllocationScope> {}; }
namespace detail { struct SymbolOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct SymbolOpInterfaceTrait;
}
class SymbolOpInterface : public ::mlir::OpInterface<SymbolOpInterface,detail::SymbolOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<SymbolOpInterface,detail::SymbolOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::SymbolOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct SymbolOpInterfaceTrait : public ::mlir::OpInterface<SymbolOpInterface,detail::SymbolOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace detail { struct ArgAndResultAttrsOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct ArgAndResultAttrsOpInterfaceTrait;
}
class ArgAndResultAttrsOpInterface : public ::mlir::OpInterface<ArgAndResultAttrsOpInterface,detail::ArgAndResultAttrsOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<ArgAndResultAttrsOpInterface,detail::ArgAndResultAttrsOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::ArgAndResultAttrsOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct ArgAndResultAttrsOpInterfaceTrait : public ::mlir::OpInterface<ArgAndResultAttrsOpInterface,detail::ArgAndResultAttrsOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace detail { struct CallableOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct CallableOpInterfaceTrait;
}
class CallableOpInterface : public ::mlir::OpInterface<CallableOpInterface,detail::CallableOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<CallableOpInterface,detail::CallableOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::CallableOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct CallableOpInterfaceTrait : public ::mlir::OpInterface<CallableOpInterface,detail::CallableOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace detail { struct FunctionOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct FunctionOpInterfaceTrait;
}
class FunctionOpInterface : public ::mlir::OpInterface<FunctionOpInterface,detail::FunctionOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<FunctionOpInterface,detail::FunctionOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::FunctionOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct FunctionOpInterfaceTrait : public ::mlir::OpInterface<FunctionOpInterface,detail::FunctionOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace OpTrait { template <typename ConcreteType> class IsIsolatedFromAbove : public TraitBase<ConcreteType,IsIsolatedFromAbove> {}; }
namespace detail { struct OpAsmOpInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct OpAsmOpInterfaceTrait;
}
class OpAsmOpInterface : public ::mlir::OpInterface<OpAsmOpInterface,detail::OpAsmOpInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<OpAsmOpInterface,detail::OpAsmOpInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::OpAsmOpInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct OpAsmOpInterfaceTrait : public ::mlir::OpInterface<OpAsmOpInterface,detail::OpAsmOpInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
namespace tosa { class ReshapeOp; }
template class Op<tosa::ReshapeOp, OpTrait::ZeroRegions, OpTrait::OneResult, OpTrait::OneTypedResult<TensorType>::Impl, OpTrait::ZeroSuccessors, OpTrait::NOperands<2>::Impl, OpTrait::OpInvariants, ConditionallySpeculatable::Trait, OpTrait::AlwaysSpeculatableImplTrait, MemoryEffectOpInterface::Trait, InferTypeOpInterface::Trait, InferShapedTypeOpInterface::Trait, OpTrait::InferShapedTypeOpAdaptor, OpTrait::InferTensorType, tosa::TosaOp::Trait, ::mlir::tosa::QueryProfileInterface::Trait, ::mlir::tosa::QueryExtensionInterface::Trait, OpTrait::tosa::TosaResolvableShapeOperands>;
class Dialect {
public:
  virtual ~Dialect();
protected:
  Dialect(StringRef name,MLIRContext *context,TypeID id);
private:
  Dialect(const Dialect&) = delete;
  void operator=(Dialect&) = delete;
  // Real dialect tables are outside current registration declarations.
};
namespace tosa {
class TosaDialect : public ::mlir::Dialect {
  explicit TosaDialect(::mlir::MLIRContext *context);
  void initialize();
  friend class ::mlir::MLIRContext;
public:
  ~TosaDialect() override;
};
class ReshapeOp : public Op<ReshapeOp, OpTrait::ZeroRegions, OpTrait::OneResult, OpTrait::OneTypedResult<TensorType>::Impl, OpTrait::ZeroSuccessors, OpTrait::NOperands<2>::Impl, OpTrait::OpInvariants, ConditionallySpeculatable::Trait, OpTrait::AlwaysSpeculatableImplTrait, MemoryEffectOpInterface::Trait, InferTypeOpInterface::Trait, InferShapedTypeOpInterface::Trait, OpTrait::InferShapedTypeOpAdaptor, OpTrait::InferTensorType, TosaOp::Trait, ::mlir::tosa::QueryProfileInterface::Trait, ::mlir::tosa::QueryExtensionInterface::Trait, OpTrait::tosa::TosaResolvableShapeOperands> {
public:
  using Op::Op;
  // Canonical type of mlir::TypedValue<TensorType> in Value.h:494.
  inline ::mlir::detail::TypedValue<::mlir::TensorType> getInput1();
  inline ::mlir::OpOperand &getInput1Mutable();
};
}

// Builders.h:51-56,209-231. Unused insertion-point methods/iterator field omitted.
class Region;
class Builder {
public:
  inline explicit Builder(MLIRContext *context);
  inline explicit Builder(Operation *op);
  inline MLIRContext *getContext() const;
protected:
  MLIRContext *context;
};
class OpBuilder : public Builder {
public:
  struct Listener;
  inline explicit OpBuilder(MLIRContext *ctx, Listener *listener = nullptr);
  inline explicit OpBuilder(Region *region, Listener *listener = nullptr);
  inline explicit OpBuilder(Region &region, Listener *listener = nullptr);
  inline explicit OpBuilder(Operation *op, Listener *listener = nullptr);
protected:
  Listener *listener;
private:
  // Block::iterator is outside current calls; omitted, not replaced by a pointer.
};
class RewriterBase : public OpBuilder {
public:
  struct Listener;
  virtual void eraseOp(Operation *op);
  virtual void replaceOp(Operation *op, ValueRange newValues);
  virtual void replaceOp(Operation *op, Operation *newOp);
  inline virtual void startOpModification(Operation *op);
  virtual void finalizeOpModification(Operation *op);

  // A definition is necessary for instantiation with a local lambda type.
  // This is the exact used wrapper body from PatternMatch.h; notification
  // methods remain declarations, with no modeled IR behavior or contracts.
  template <typename CallableT>
  void modifyOpInPlace(Operation *root, CallableT &&callable) {
    startOpModification(root);
    callable();
    finalizeOpModification(root);
  }
protected:
  inline explicit RewriterBase(MLIRContext *ctx, OpBuilder::Listener *listener = nullptr);
  inline explicit RewriterBase(const OpBuilder &otherBuilder);
  inline explicit RewriterBase(Operation *op, OpBuilder::Listener *listener = nullptr);
  virtual ~RewriterBase();
private:
  void operator=(const RewriterBase &) = delete;
  RewriterBase(const RewriterBase &) = delete;

};
class PatternRewriter : public RewriterBase {
public:
  inline explicit PatternRewriter(MLIRContext *ctx);
  using RewriterBase::RewriterBase;
  inline virtual bool canRecoverFromRewriteFailure() const;
};

class PatternBenefit {
  enum { ImpossibleToMatchSentinel = 65535 };
public:
  PatternBenefit() = default;
  PatternBenefit(unsigned benefit);
  PatternBenefit(const PatternBenefit &) = default;
  PatternBenefit &operator=(const PatternBenefit &) = default;
private:
  unsigned short representation{ImpossibleToMatchSentinel};
};

class OperationName {
public:
  class Impl;
  OperationName(StringRef name,MLIRContext *context);
protected:
  OperationName(Impl *impl);
private:
  Impl *impl = nullptr;
};
}
template class llvm::SmallVector<mlir::OperationName,2>;
namespace mlir {
class Pattern {
  enum class RootKind { Any, OperationName, InterfaceID, TraitID };
protected:
  Pattern(StringRef rootName, PatternBenefit benefit, MLIRContext *context,
          ArrayRef<StringRef> generatedNames = {});
private:
  const void *rootValue;
  RootKind rootKind;
  const PatternBenefit benefit;
  llvm::PointerIntPair<MLIRContext*,1,bool> contextAndHasBoundedRecursion;
  SmallVector<OperationName,2> generatedOps;
  StringRef debugName;
  SmallVector<StringRef,0> debugLabels;

};

class RewritePattern : public Pattern {
public:
  virtual ~RewritePattern() = default;
  virtual LogicalResult matchAndRewrite(Operation *op,
                                        PatternRewriter &rewriter) const = 0;
protected:
  using Pattern::Pattern;
private:
  virtual void anchor();

};

namespace detail {
template <typename SourceOp>
struct OpOrInterfaceRewritePatternBase : public RewritePattern {
  using RewritePattern::RewritePattern;
  inline LogicalResult matchAndRewrite(Operation *op,
                                PatternRewriter &rewriter) const final;
  virtual LogicalResult matchAndRewrite(SourceOp op,
                                        PatternRewriter &rewriter) const = 0;
};
}

template <typename SourceOp>
struct OpRewritePattern
    : public mlir::detail::OpOrInterfaceRewritePatternBase<SourceOp> {
  using Base = OpRewritePattern;
  inline OpRewritePattern(MLIRContext *context, PatternBenefit benefit = 1,
                   ArrayRef<StringRef> generatedNames = {});
};

enum class GreedyRewriteStrictness { AnyOp, ExistingAndNewOps, ExistingOps };
enum class GreedySimplifyRegionLevel { Disabled, Normal, Aggressive };
class GreedyRewriteConfig {
public:
  GreedyRewriteConfig &setStrictness(GreedyRewriteStrictness mode) { strictness = mode; return *this; }
  GreedyRewriteConfig &enableConstantCSE(bool enable = true) { cseConstants = enable; return *this; }
  GreedyRewriteConfig &enableFolding(bool enable = true) { fold = enable; return *this; }
  GreedyRewriteConfig &setRegionSimplificationLevel(GreedySimplifyRegionLevel level) { regionSimplificationLevel = level; return *this; }
  static constexpr long kNoLimit = -1;
private:
  Region *scope = nullptr;
  bool useTopDownTraversal = false;
  GreedySimplifyRegionLevel regionSimplificationLevel = GreedySimplifyRegionLevel::Aggressive;
  long maxIterations = 10;
  long maxNumRewrites = kNoLimit;
  GreedyRewriteStrictness strictness = GreedyRewriteStrictness::AnyOp;
  RewriterBase::Listener *listener = nullptr;
  bool fold = true;
  bool cseConstants = true;
  bool cseBetweenIterations = false;

};

namespace OpTrait { template <typename ConcreteType> struct NoRegionArguments : public TraitBase<ConcreteType,NoRegionArguments> {}; }
namespace OpTrait { template <typename ConcreteType> class NoTerminator : public TraitBase<ConcreteType,NoTerminator> {}; }
namespace OpTrait { template <typename ConcreteType> struct SingleBlock : public TraitBase<ConcreteType,SingleBlock> {}; }
namespace OpTrait { template <typename ConcreteType> class SymbolTable : public TraitBase<ConcreteType,SymbolTable> {}; }
namespace OpTrait { template <typename ConcreteType> class HasOnlyGraphRegion : public TraitBase<ConcreteType,HasOnlyGraphRegion> {}; }
namespace detail { struct RegionKindInterfaceInterfaceTraits { struct Concept; template <typename T> class Model; template <typename T> class FallbackModel; template <typename T,typename U> class ExternalModel; };
template <typename ConcreteOp> struct RegionKindInterfaceTrait;
}
class RegionKindInterface : public ::mlir::OpInterface<RegionKindInterface,detail::RegionKindInterfaceInterfaceTraits> {
public:
 using ::mlir::OpInterface<RegionKindInterface,detail::RegionKindInterfaceInterfaceTraits>::OpInterface;
 template <typename ConcreteOp> struct Trait : public detail::RegionKindInterfaceTrait<ConcreteOp> {};
};
namespace detail { template <typename ConcreteOp> struct RegionKindInterfaceTrait : public ::mlir::OpInterface<RegionKindInterface,detail::RegionKindInterfaceInterfaceTraits>::template Trait<ConcreteOp> {}; }
class ModuleOp;
template class ::mlir::Op<ModuleOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::ZeroOperands, ::mlir::OpTrait::NoRegionArguments, ::mlir::OpTrait::NoTerminator, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AffineScope, ::mlir::OpTrait::IsIsolatedFromAbove, ::mlir::OpTrait::SymbolTable, ::mlir::SymbolOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait, ::mlir::RegionKindInterface::Trait, ::mlir::OpTrait::HasOnlyGraphRegion>;
class ModuleOp : public ::mlir::Op<ModuleOp, ::mlir::OpTrait::OneRegion, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::ZeroOperands, ::mlir::OpTrait::NoRegionArguments, ::mlir::OpTrait::NoTerminator, ::mlir::OpTrait::SingleBlock, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpTrait::AffineScope, ::mlir::OpTrait::IsIsolatedFromAbove, ::mlir::OpTrait::SymbolTable, ::mlir::SymbolOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait, ::mlir::RegionKindInterface::Trait, ::mlir::OpTrait::HasOnlyGraphRegion> {
public:
 using Op::Op;
};
// OwningOpRef.h:28-91; original field and special-member declarations.
template <typename OpTy> class OwningOpRef {
public:
 using OperationT = OpTy;
 OwningOpRef(std::nullptr_t = nullptr);
 OwningOpRef(OpTy op);
 OwningOpRef(OwningOpRef &&other);
 ~OwningOpRef();
 OwningOpRef &operator=(OwningOpRef &&other);
private:
 OpTy op;
};
// PDL is enabled in this build. Keep the owning-module member that determines
// deleted copying and non-trivial destruction. Unused configs/maps/functions
// are explicitly abstracted, not replaced by different-typed fields.
class PDLPatternModule {
public:
 PDLPatternModule() = default;
 PDLPatternModule(OwningOpRef<ModuleOp> module);
 template <typename... ConfigsT>
 PDLPatternModule(OwningOpRef<ModuleOp> module,ConfigsT&&... patternConfigs);
private:
 OwningOpRef<ModuleOp> pdlModule;
};
class RewritePatternSet {
  using NativePatternListT = std::vector<std::unique_ptr<RewritePattern>>;
public:
  inline RewritePatternSet(MLIRContext *context);
  // Real candidate declarations/constraint. addImpl/create/allocation bodies remain external.
  template <typename... Ts, typename ConstructorArg, typename... ConstructorArgs,
            typename = std::enable_if_t<sizeof...(Ts) != 0>>
  inline RewritePatternSet &add(ConstructorArg &&arg, ConstructorArgs &&...args);
  template <typename... Ts> inline RewritePatternSet &add();
  inline RewritePatternSet &add(std::unique_ptr<RewritePattern> pattern);
  inline RewritePatternSet &add(PDLPatternModule &&pattern);
  template <typename OpType>
  inline RewritePatternSet &add(LogicalResult (*implFn)(OpType,PatternRewriter&),
                        PatternBenefit benefit = 1,ArrayRef<StringRef> generatedNames = {});

private:
  MLIRContext *const context;
  NativePatternListT nativePatterns;
  PDLPatternModule pdlPatterns;

};

class FrozenRewritePatternSet {
public:
  FrozenRewritePatternSet();
  FrozenRewritePatternSet(FrozenRewritePatternSet &&patterns) = default;
  FrozenRewritePatternSet(const FrozenRewritePatternSet &patterns) = default;
  FrozenRewritePatternSet &operator=(const FrozenRewritePatternSet &patterns) = default;
  FrozenRewritePatternSet &operator=(FrozenRewritePatternSet &&patterns) = default;
  FrozenRewritePatternSet(RewritePatternSet &&patterns,
                         ArrayRef<std::string> disabledPatternLabels = {},
                         ArrayRef<std::string> enabledPatternLabels = {});
  ~FrozenRewritePatternSet();
private:
  struct Impl;
  std::shared_ptr<Impl> impl;

};

LogicalResult applyOpPatternsGreedily(
    ArrayRef<Operation *> ops, const FrozenRewritePatternSet &patterns,
    GreedyRewriteConfig config = GreedyRewriteConfig(),
    bool *changed = nullptr, bool *allErased = nullptr);

// Interfaces extracted from Pass.h. No execution or clone implementation.
class Pass;
using PassAllocatorFunction = std::function<std::unique_ptr<Pass>()>;
template <typename ConcretePass> struct PassRegistration {
  inline PassRegistration(const PassAllocatorFunction &constructor);
  inline PassRegistration();
};

class Dialect;
using DialectAllocatorFunction = std::function<Dialect*(MLIRContext*)>;
class DialectRegistry {
  using MapTy = std::map<std::string,std::pair<TypeID,DialectAllocatorFunction>,std::less<>>;
public:
  explicit DialectRegistry();
  DialectRegistry(const DialectRegistry&) = delete;
  DialectRegistry& operator=(const DialectRegistry& other) = delete;
  DialectRegistry(DialectRegistry&&) = default;
  DialectRegistry& operator=(DialectRegistry&& other) = default;
  template <typename ConcreteDialect> inline void insert();
  template <typename ConcreteDialect,typename OtherDialect,typename... MoreDialects>
  inline void insert();
private:
  MapTy registry;
  // Unused preload-name vector and extension MapVector storage omitted.
};
class TypeID {
  struct alignas(8) Storage {};
public:
  inline TypeID();
  inline bool operator==(const TypeID &other) const;
  inline bool operator!=(const TypeID &other) const;
  template <typename T> static TypeID get();
  template <template<typename> class Trait> static TypeID get();
  inline const void *getAsOpaquePointer() const;
  inline static TypeID getFromOpaquePointer(const void *pointer);
private:
  inline TypeID(const Storage *storage);
  const Storage *storage;
};
// TypeID.h:369-384: real self-owned ID declaration/short wrappers.
class alignas(8) SelfOwningTypeID {
public:
  SelfOwningTypeID() = default;
  SelfOwningTypeID(const SelfOwningTypeID&) = delete;
  SelfOwningTypeID& operator=(const SelfOwningTypeID&) = delete;
  SelfOwningTypeID(SelfOwningTypeID&&) = delete;
  SelfOwningTypeID& operator=(SelfOwningTypeID&&) = delete;
  operator TypeID() const { return getTypeID(); }
  TypeID getTypeID() const { return TypeID::getFromOpaquePointer(this); }
};
namespace detail {
struct InlineTypeIDResolver {
  template <typename T> using has_resolve_typeid_trait = decltype(T::resolveTypeID());
  template <typename T> using has_resolve_typeid = llvm::is_detected<has_resolve_typeid_trait,T>;
  template <typename T> static TypeID resolveTypeID() { return T::resolveTypeID(); }
};
}
class Pass {
public:
  virtual ~Pass() = default;
  virtual StringRef getName() const = 0;
  inline virtual StringRef getArgument() const;
  inline virtual StringRef getDescription() const;
  inline virtual void getDependentDialects(DialectRegistry &registry) const;
protected:
  virtual std::unique_ptr<Pass> clonePass() const = 0;
  virtual void runOnOperation() = 0;
  inline Pass(const Pass &other);
  Pass &operator=(const Pass &) = delete;
  Pass(Pass &&) = delete;
  Pass &operator=(Pass &&) = delete;
};
namespace func {
class FuncOp : public Op<FuncOp, OpTrait::OneRegion, OpTrait::ZeroResults, OpTrait::ZeroSuccessors, OpTrait::ZeroOperands, OpTrait::OpInvariants, BytecodeOpInterface::Trait, OpTrait::AffineScope, OpTrait::AutomaticAllocationScope, SymbolOpInterface::Trait, ArgAndResultAttrsOpInterface::Trait, CallableOpInterface::Trait, FunctionOpInterface::Trait, OpTrait::IsIsolatedFromAbove, OpAsmOpInterface::Trait> {
public:
  using Op::Op;
};
}
template <typename OpT = void>
class OperationPass : public Pass {
public:
  ~OperationPass() override = default;
protected:
  inline OpT getOperation();
  inline OperationPass(TypeID passID);
  OperationPass(const OperationPass &) = default;
  OperationPass &operator=(const OperationPass &) = delete;
  OperationPass(OperationPass &&) = delete;
  OperationPass &operator=(OperationPass &&) = delete;
};
template <typename PassT, typename BaseT>
class PassWrapper : public BaseT {
public:
  ~PassWrapper() override = default;
protected:
  inline PassWrapper();
  PassWrapper(const PassWrapper &) = default;
  PassWrapper &operator=(const PassWrapper &) = delete;
  PassWrapper(PassWrapper &&) = delete;
  PassWrapper &operator=(PassWrapper &&) = delete;
  inline StringRef getName() const override;
  inline std::unique_ptr<Pass> clonePass() const override;

};
}

// Exact macro from the configured TypeID.h.
#define MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CLASS_NAME)               \
  static ::mlir::TypeID resolveTypeID() {                                      \
    static ::mlir::SelfOwningTypeID id;                                        \
    return id;                                                                 \
  }                                                                            \
  static_assert(                                                               \
      ::mlir::detail::InlineTypeIDResolver::has_resolve_typeid<                \
          CLASS_NAME>::value,                                                  \
      "`MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID` must be placed in a "    \
      "public section of `" #CLASS_NAME "`");

#endif
