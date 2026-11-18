// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Shared bounded interface model. Edit here; pass libraries are generated.
#ifndef BUDDY_MINI_MINI_SUPPORT_STANDARD_H
#define BUDDY_MINI_MINI_SUPPORT_STANDARD_H

typedef long int64_t;
 
 
 
// Provenance: audited Reshape V2 mini, line 11; see docs/PROVENANCE.md.
typedef unsigned long size_t;
// Provenance: audited Reshape V2 mini, line 18; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct char_traits;
} // namespace std
// Provenance: audited Reshape V2 mini, line 19; see docs/PROVENANCE.md.
namespace std {
template <typename T> class allocator;
} // namespace std
// Provenance: audited Reshape V2 mini, line 21; see docs/PROVENANCE.md.
namespace std {
inline namespace __cxx11 {
template <typename CharT, typename Traits = char_traits<CharT>,
          typename Allocator = allocator<CharT>>
class basic_string;
} // namespace __cxx11
} // namespace std
// Provenance: audited Reshape V2 mini, line 23; see docs/PROVENANCE.md.
namespace std {
using string = __cxx11::basic_string<char, char_traits<char>, allocator<char>>;
} // namespace std
// Provenance: audited Reshape V2 mini, line 24; see docs/PROVENANCE.md.
namespace std {
template <typename CharT, typename Traits = char_traits<CharT>>
class basic_string_view;
} // namespace std
// Provenance: audited Reshape V2 mini, line 25; see docs/PROVENANCE.md.
namespace std {
using string_view = basic_string_view<char, char_traits<char>>;
} // namespace std
// Provenance: audited Reshape V2 mini, line 26; see docs/PROVENANCE.md.
namespace std {
using nullptr_t = decltype(nullptr);
} // namespace std
// Provenance: audited Reshape V2 mini, line 27; see docs/PROVENANCE.md.
namespace std {
template <class E> class initializer_list {
public:
  using value_type = E;
  using reference = const E &;
  using const_reference = const E &;
  using size_type = size_t;
  using iterator = const E *;
  using const_iterator = const E *;

private:
  iterator _M_array;
  size_type _M_len;
  constexpr initializer_list(const_iterator, size_type);

public:
  constexpr initializer_list() noexcept : _M_array(nullptr), _M_len(0) {}
  constexpr size_type size() const noexcept;
   
   
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 46; see docs/PROVENANCE.md.
namespace std {
template <class T, T v> struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant<T, v>;
  constexpr operator value_type() const noexcept { return value; }
   
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 53; see docs/PROVENANCE.md.
namespace std {
using true_type = integral_constant<bool, true>;
} // namespace std
// Provenance: audited Reshape V2 mini, line 54; see docs/PROVENANCE.md.
namespace std {
using false_type = integral_constant<bool, false>;
} // namespace std
// Provenance: audited Reshape V2 mini, line 55; see docs/PROVENANCE.md.
namespace std {
template <bool V> using bool_constant = integral_constant<bool, V>;
} // namespace std
// Provenance: audited Reshape V2 mini, line 56; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_reference {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 57; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_reference<T &> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 58; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_reference<T &&> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 59; see docs/PROVENANCE.md.
namespace std {
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;
} // namespace std
// Provenance: audited Reshape V2 mini, line 60; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_const {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 61; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_const<const T> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 62; see docs/PROVENANCE.md.
namespace std {
 
template <typename T> struct remove_volatile {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 63; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_volatile<volatile T> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 64; see docs/PROVENANCE.md.
namespace std {
template <typename T>
struct remove_cv : remove_const<typename remove_volatile<T>::type> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 65; see docs/PROVENANCE.md.
namespace std {
template <typename T> using remove_cv_t = typename remove_cv<T>::type;
} // namespace std
// Provenance: audited Reshape V2 mini, line 66; see docs/PROVENANCE.md.
namespace std {
template <bool, class T = void> struct enable_if {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 67; see docs/PROVENANCE.md.
namespace std {
template <class T> struct enable_if<true, T> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 68; see docs/PROVENANCE.md.
namespace std {
template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;
} // namespace std
// Provenance: audited Reshape V2 mini, line 69; see docs/PROVENANCE.md.
namespace std {
template <bool B, class T, class F> struct conditional {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 70; see docs/PROVENANCE.md.
namespace std {
template <class T, class F> struct conditional<false, T, F> {
  using type = F;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 71; see docs/PROVENANCE.md.
namespace std {
template <bool B, class T, class F>
using conditional_t = typename conditional<B, T, F>::type;
} // namespace std
// Provenance: audited Reshape V2 mini, line 72; see docs/PROVENANCE.md.
namespace std {
template <class...> using void_t = void;
} // namespace std
// Provenance: audited Reshape V2 mini, line 73; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename... Args>
struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 74; see docs/PROVENANCE.md.
namespace std {
template <class T> struct is_class : bool_constant<__is_class(T)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 75; see docs/PROVENANCE.md.
namespace std {
template <class B, class D>
struct is_base_of : bool_constant<__is_base_of(B, D)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 76; see docs/PROVENANCE.md.
namespace std {
template <class T, class U> struct is_same : false_type {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 77; see docs/PROVENANCE.md.
namespace std {
template <class T> struct is_same<T, T> : true_type {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 78; see docs/PROVENANCE.md.
namespace std {
template <class T> struct is_void : is_same<void, remove_cv_t<T>> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 79; see docs/PROVENANCE.md.
namespace std {
template <class T> struct is_integral : bool_constant<__is_integral(T)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 80; see docs/PROVENANCE.md.
namespace std {
template <class T, class U>
struct is_convertible : bool_constant<__is_convertible(T, U)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 81; see docs/PROVENANCE.md.
namespace std {
template <class T, class U>
struct is_assignable : bool_constant<__is_assignable(T, U)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 82; see docs/PROVENANCE.md.
namespace std {
template <class T>
struct is_trivially_copy_constructible
    : bool_constant<__is_trivially_constructible(T, const T &)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 83; see docs/PROVENANCE.md.
namespace std {
template <class T>
struct is_trivially_move_constructible
    : bool_constant<__is_trivially_constructible(T, T &&)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 84; see docs/PROVENANCE.md.
namespace std {
template <class T>
struct is_trivially_destructible
    : bool_constant<__is_trivially_destructible(T)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 85; see docs/PROVENANCE.md.
namespace std {
template <class...> struct conjunction : true_type {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 86; see docs/PROVENANCE.md.
namespace std {
template <class B> struct conjunction<B> : B {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 87; see docs/PROVENANCE.md.
namespace std {
template <class B, class... Bs>
struct conjunction<B, Bs...>
    : conditional_t<bool(B::value), conjunction<Bs...>, B> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 88; see docs/PROVENANCE.md.
namespace std {
 


} // namespace std
// Provenance: audited Reshape V2 mini, line 91; see docs/PROVENANCE.md.
namespace std {
 
} // namespace std
// Provenance: audited Reshape V2 mini, line 92; see docs/PROVENANCE.md.
namespace std {
template <class T> T &&declval() noexcept;
} // namespace std
// Provenance: audited Reshape V2 mini, line 93; see docs/PROVENANCE.md.
namespace std {
template <typename T>
constexpr typename std::remove_reference<T>::type &&move(T &&value) noexcept {
  return static_cast<typename std::remove_reference<T>::type &&>(value);
}
} // namespace std
// Provenance: audited Reshape V2 mini, line 95; see docs/PROVENANCE.md.
namespace std {
 



} // namespace std
// Provenance: audited Reshape V2 mini, line 97; see docs/PROVENANCE.md.
namespace std {
 




} // namespace std
// Provenance: audited Reshape V2 mini, line 101; see docs/PROVENANCE.md.
namespace std {
 
} // namespace std
// Provenance: audited Reshape V2 mini, line 102; see docs/PROVENANCE.md.
namespace std {
 
} // namespace std
// Provenance: audited Reshape V2 mini, line 103; see docs/PROVENANCE.md.
namespace std {
 


} // namespace std
// Provenance: audited Reshape V2 mini, line 105; see docs/PROVENANCE.md.
namespace std {
 


} // namespace std
// Provenance: audited Reshape V2 mini, line 106; see docs/PROVENANCE.md.
namespace std {
 

} // namespace std
// Provenance: audited Reshape V2 mini, line 107; see docs/PROVENANCE.md.
namespace std {
template <typename B> struct negation : bool_constant<!bool(B::value)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 108; see docs/PROVENANCE.md.
namespace std {
template <typename...> struct disjunction : false_type {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 109; see docs/PROVENANCE.md.
namespace std {
template <typename B> struct disjunction<B> : B {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 110; see docs/PROVENANCE.md.
namespace std {
template <typename B, typename... Bs>
struct disjunction<B, Bs...>
    : conditional_t<bool(B::value), B, disjunction<Bs...>> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 117; see docs/PROVENANCE.md.
namespace std {
template <typename T> class __new_allocator {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 118; see docs/PROVENANCE.md.
namespace std {
template <typename T> class allocator : public __new_allocator<T> {
public:
  allocator() noexcept;
  allocator(const allocator &) noexcept;
  template <typename U> allocator(const allocator<U> &) noexcept;
  ~allocator() noexcept;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 125; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct default_delete {
  constexpr default_delete() noexcept = default;
   
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 129; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename D> class __uniq_ptr_impl {
public:
  using pointer = T *;
  __uniq_ptr_impl() = default;
  __uniq_ptr_impl(__uniq_ptr_impl &&) noexcept;
  __uniq_ptr_impl &operator=(__uniq_ptr_impl &&) noexcept;
  // The real tuple<pointer,D> storage is opaque here; no layout claim.
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 137; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename D, bool = __is_constructible(D, D &&),
          bool = __is_assignable(D &, D &&)>
struct __uniq_ptr_data : __uniq_ptr_impl<T, D> {
  using __uniq_ptr_impl<T, D>::__uniq_ptr_impl;
  __uniq_ptr_data(__uniq_ptr_data &&) = default;
  __uniq_ptr_data &operator=(__uniq_ptr_data &&) = default;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 144; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename D = default_delete<T>> class unique_ptr {
  __uniq_ptr_data<T, D> _M_t;

public:
  using pointer = typename __uniq_ptr_impl<T, D>::pointer;
  using element_type = T;
  using deleter_type = D;
  template <typename Del = D, typename = enable_if_t<!__is_pointer(Del) &&
                                                     __is_constructible(Del)>>
  constexpr unique_ptr() noexcept;
  unique_ptr(unique_ptr &&) = default;
  unique_ptr &operator=(unique_ptr &&) = default;
  ~unique_ptr() noexcept;
  unique_ptr(const unique_ptr &) = delete;
  unique_ptr &operator=(const unique_ptr &) = delete;
};
} // namespace std
namespace std { template<typename T> struct iterator_traits; }
namespace __gnu_cxx {
template<typename Iterator,typename Container> class __normal_iterator {
protected: Iterator _M_current;
public:
 using iterator_category=typename std::iterator_traits<Iterator>::iterator_category;
 using value_type=typename std::iterator_traits<Iterator>::value_type;
 using difference_type=typename std::iterator_traits<Iterator>::difference_type;
 using pointer=typename std::iterator_traits<Iterator>::pointer;
 using reference=typename std::iterator_traits<Iterator>::reference;
 __normal_iterator();    
};
 
}
// Provenance: audited Reshape V2 mini, line 158; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename Alloc> struct _Vector_base {
 struct _Vector_impl_data { T *_M_start; T *_M_finish; T *_M_end_of_storage; };
 _Vector_impl_data _M_impl;

  _Vector_base() = default;
  _Vector_base(_Vector_base &&) = default;
  ~_Vector_base() noexcept;
  // Real allocator and three-pointer storage is outside current layout use.
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 164; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename Alloc = allocator<T>>
class vector : protected _Vector_base<T, Alloc> {
public:
  using size_type = size_t;
  using iterator = __gnu_cxx::__normal_iterator<T *, vector>;
     
      size_type size() const;
   
  vector();
  vector(const vector &);
  vector(vector &&) noexcept = default;
  vector &operator=(const vector &);
  vector &operator=(vector &&) noexcept;
  ~vector() noexcept;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 175; see docs/PROVENANCE.md.
namespace __gnu_cxx {
enum _Lock_policy { _S_single, _S_mutex, _S_atomic };
} // namespace __gnu_cxx
// Provenance: audited Reshape V2 mini, line 177; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct is_array : bool_constant<__is_array(T)> {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 178; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_extent {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 179; see docs/PROVENANCE.md.
namespace std {
template <typename T> struct remove_extent<T[]> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 180; see docs/PROVENANCE.md.
namespace std {
template <typename T, size_t N> struct remove_extent<T[N]> {
  using type = T;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 181; see docs/PROVENANCE.md.
namespace std {
template <__gnu_cxx::_Lock_policy L> class _Sp_counted_base;
} // namespace std
// Provenance: audited Reshape V2 mini, line 182; see docs/PROVENANCE.md.
namespace std {
template <__gnu_cxx::_Lock_policy L = __gnu_cxx::_S_atomic>
class __shared_count {
public:
  constexpr __shared_count() noexcept : _M_pi(0) {}
  __shared_count(const __shared_count &) noexcept;
  __shared_count &operator=(const __shared_count &) noexcept;
  ~__shared_count() noexcept;

private:
  _Sp_counted_base<L> *_M_pi;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 191; see docs/PROVENANCE.md.
namespace std {
template <typename T, __gnu_cxx::_Lock_policy L, bool = is_array<T>::value,
          bool = is_void<T>::value>
class __shared_ptr_access {};
} // namespace std
// Provenance: audited Reshape V2 mini, line 194; see docs/PROVENANCE.md.
namespace std {
template <typename T, __gnu_cxx::_Lock_policy L = __gnu_cxx::_S_atomic>
class __shared_ptr : public __shared_ptr_access<T, L> {
public:
  using element_type = typename remove_extent<T>::type;
  constexpr __shared_ptr() noexcept;
  __shared_ptr(const __shared_ptr &) noexcept = default;
  __shared_ptr(__shared_ptr &&) noexcept;
  __shared_ptr &operator=(const __shared_ptr &) noexcept = default;
  __shared_ptr &operator=(__shared_ptr &&) noexcept;
  ~__shared_ptr() = default;

private:
  element_type *_M_ptr;
  __shared_count<L> _M_refcount;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 208; see docs/PROVENANCE.md.
namespace std {
template <typename T> class shared_ptr : public __shared_ptr<T> {
public:
  using element_type = typename __shared_ptr<T>::element_type;
  constexpr shared_ptr() noexcept;
  shared_ptr(const shared_ptr &) noexcept = default;
  shared_ptr(shared_ptr &&) noexcept;
  shared_ptr &operator=(const shared_ptr &) noexcept = default;
  shared_ptr &operator=(shared_ptr &&) noexcept;
};
} // namespace std
// Provenance: audited Reshape V2 mini, line 221; see docs/PROVENANCE.md.
extern "C" void __assert_fail(const char *, const char *, unsigned int,
                              const char *) noexcept __attribute__((noreturn));
// Provenance: audited Reshape V2 mini, line 224; see docs/PROVENANCE.md.
namespace std {
template <typename T, typename U> struct pair { T first; U second; };
} // namespace std
// Provenance: audited Reshape V2 mini, line 225; see docs/PROVENANCE.md.
namespace std {
template <typename T = void> struct less;
} // namespace std
// Provenance: audited Reshape V2 mini, line 226; see docs/PROVENANCE.md.
namespace std {
template <typename Signature> class function;
} // namespace std
// Provenance: audited Reshape V2 mini, line 227; see docs/PROVENANCE.md.
namespace std {
template <typename Key, typename T, typename Compare = less<Key>,
          typename Alloc = allocator<pair<const Key, T>>>
class map {
public:
  map() = default;
  map(const map &) = default;
  map(map &&) = default;
  map &operator=(const map &) = default;
  map &operator=(map &&) = default;
  ~map();
  // Red-black tree storage is not modeled; no map operation is invoked here.
};
} // namespace std
#define MINI_LLVM_ASSERT(expr)                                                 \
  (static_cast<bool>(expr)                                                     \
       ? void(0)                                                               \
       : __assert_fail(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__))

namespace std {
struct input_iterator_tag {};
struct forward_iterator_tag : input_iterator_tag {};
struct bidirectional_iterator_tag : forward_iterator_tag {};
struct random_access_iterator_tag : bidirectional_iterator_tag {};
using ptrdiff_t = long;
template<typename It, typename = void> struct __iterator_traits {};
template<typename It> struct __iterator_traits<It, void_t<typename It::iterator_category, typename It::value_type, typename It::difference_type, typename It::pointer, typename It::reference>> {
 using iterator_category=typename It::iterator_category; using value_type=typename It::value_type;
 using difference_type=typename It::difference_type; using pointer=typename It::pointer; using reference=typename It::reference;
};
template<typename T> struct iterator_traits : __iterator_traits<T> {};
template<typename T> struct iterator_traits<T *> {
 using iterator_category=random_access_iterator_tag; using value_type=remove_cv_t<T>;
 using difference_type=ptrdiff_t; using pointer=T *; using reference=T &;
};
}

namespace std {
struct nullopt_t { enum class _Construct { _Token }; explicit constexpr nullopt_t(_Construct) {} };
 
template<typename T> class optional {
public:
 constexpr optional() noexcept;
 constexpr optional(nullopt_t) noexcept;
 optional(const optional &);
 optional(optional &&);
 optional &operator=(const optional &);
 optional &operator=(optional &&);
 ~optional();
private:
 // Bounded storage model, not the libstdc++ helper inheritance/ABI.
 union { unsigned char empty; T value; } payload;
 bool engaged;
};
}
#endif
