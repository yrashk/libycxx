// libycxx core: <variant> ([variant]).
//
// Storage is a recursive union with conditionally trivial special members. Changing the active
// alternative re-constructs the whole union with construct_at(&u_, in_place_index<I>, ...),
// which is valid both at run time and in constant evaluation (activating a nested inactive
// union member directly is not).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/swap.hpp>
#include <initializer_list>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class bad_variant_access : public exception {
public:
  constexpr bad_variant_access() noexcept {}
  constexpr bad_variant_access(const bad_variant_access&) noexcept = default;
  constexpr bad_variant_access& operator=(const bad_variant_access&) noexcept = default;
  constexpr ~bad_variant_access() override {}
  constexpr const char* what() const noexcept override { return "bad variant access"; }
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_bad_variant_access() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_variant_access, "std::bad_variant_access", [] { return std::bad_variant_access(); });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class... _Types>
class variant;

template <class _Tp>
struct variant_size;
template <class _Tp>
struct variant_size<const _Tp> : variant_size<_Tp> {};
// [depr.variant] (Annex D); the members repeat the attribute for GCC (see tuple_like.hpp).
template <class _Tp>
struct [[deprecated("variant_size<volatile T> is deprecated ([depr.variant])")]] variant_size<volatile _Tp>
    : integral_constant<size_t, variant_size<_Tp>::value> {
  [[deprecated("variant_size<volatile T> is deprecated ([depr.variant])")]]
  static constexpr size_t value = variant_size<_Tp>::value;
};
template <class _Tp>
struct [[deprecated("variant_size<const volatile T> is deprecated ([depr.variant])")]] variant_size<const volatile _Tp>
    : integral_constant<size_t, variant_size<_Tp>::value> {
  [[deprecated("variant_size<const volatile T> is deprecated ([depr.variant])")]]
  static constexpr size_t value = variant_size<_Tp>::value;
};
template <class _Tp>
constexpr size_t variant_size_v = variant_size<_Tp>::value;
template <class... _Types>
struct variant_size<variant<_Types...>> : integral_constant<size_t, sizeof...(_Types)> {};

template <size_t _Ip, class _Tp>
struct variant_alternative;
template <size_t _Ip, class _Tp>
struct variant_alternative<_Ip, const _Tp> {
  using type = const typename variant_alternative<_Ip, _Tp>::type;
};
template <size_t _Ip, class _Tp>
struct [[deprecated("variant_alternative<I, volatile T> is deprecated ([depr.variant])")]] variant_alternative<_Ip, volatile _Tp> {
  using type [[deprecated("variant_alternative<I, volatile T> is deprecated ([depr.variant])")]] =
      volatile typename variant_alternative<_Ip, _Tp>::type;
};
template <size_t _Ip, class _Tp>
struct [[deprecated("variant_alternative<I, const volatile T> is deprecated ([depr.variant])")]]
    variant_alternative<_Ip, const volatile _Tp> {
  using type [[deprecated("variant_alternative<I, const volatile T> is deprecated ([depr.variant])")]] =
      const volatile typename variant_alternative<_Ip, _Tp>::type;
};
template <size_t _Ip, class _Tp>
using variant_alternative_t = typename variant_alternative<_Ip, _Tp>::type;
template <size_t _Ip, class... _Types>
struct variant_alternative<_Ip, variant<_Types...>> {
  static_assert(_Ip < sizeof...(_Types), "variant_alternative index out of range");
  using type = _Types...[_Ip];
};

inline constexpr size_t variant_npos = static_cast<size_t>(-1);

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// ---- index dispatch ---------------------------------------------------------------------------
// Calls f(integral_constant<size_t, i>{}) for a run-time i < N. Small N uses a compare chain,
// which both compilers inline (GCC keeps the indirect call through a table); larger N uses a
// table of function pointers. Both work in constant evaluation.
inline constexpr std::size_t __dispatch_chain_max = 12;

template <std::size_t _Ip, std::size_t _Np, class _Rp, class _Fp>
constexpr _Rp __dispatch_chain(std::size_t i, _Fp&& __f) {
  if constexpr (_Ip + 1 == _Np)
    return static_cast<_Fp&&>(__f)(std::integral_constant<std::size_t, _Ip>{});
  else if (i == _Ip)
    return static_cast<_Fp&&>(__f)(std::integral_constant<std::size_t, _Ip>{});
  else
    return ::__ycxx::__detail::__dispatch_chain<_Ip + 1, _Np, _Rp>(i, static_cast<_Fp&&>(__f));
}

template <std::size_t _Np, class _Fp>
constexpr decltype(auto) __dispatch_index(std::size_t i, _Fp&& __f) {
  using _Rp = decltype(static_cast<_Fp&&>(__f)(std::integral_constant<std::size_t, 0>{}));
  if constexpr (_Np <= __dispatch_chain_max)
    return ::__ycxx::__detail::__dispatch_chain<0, _Np, _Rp>(i, static_cast<_Fp&&>(__f));
  else
    return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) -> _Rp {
    static constexpr _Rp (*table[])(_Fp&&) = {+[](_Fp&& __g) -> _Rp {
      return static_cast<_Fp&&>(__g)(std::integral_constant<std::size_t, _Ip>{});
    }...};
      return table[i](static_cast<_Fp&&>(__f));
    }(std::make_index_sequence<_Np>{});
}

// ---- storage ------------------------------------------------------------------------------------
struct __valueless_tag {};

template <bool _TrivialDtor, class... _Ts>
union __var_union;

template <bool _TD>
union __var_union<_TD> {
  constexpr __var_union(__valueless_tag) noexcept {}
};

template <class _Tp, class... _Ts>
union __var_union<true, _Tp, _Ts...> {
  _Tp __head;
  __var_union<true, _Ts...> __tail;

  constexpr __var_union(__valueless_tag) noexcept : __tail(__valueless_tag{}) {}
  template <class... _Args>
  constexpr explicit __var_union(std::in_place_index_t<0>, _Args&&... __args) : __head(static_cast<_Args&&>(__args)...) {}
  template <std::size_t _Ip, class... _Args>
    requires(_Ip > 0)
  constexpr explicit __var_union(std::in_place_index_t<_Ip>, _Args&&... __args)
      : __tail(std::in_place_index<_Ip - 1>, static_cast<_Args&&>(__args)...) {}
};

template <class _Tp, class... _Ts>
union __var_union<false, _Tp, _Ts...> {
  _Tp __head;
  __var_union<false, _Ts...> __tail;

  constexpr __var_union(__valueless_tag) noexcept : __tail(__valueless_tag{}) {}
  template <class... _Args>
  constexpr explicit __var_union(std::in_place_index_t<0>, _Args&&... __args) : __head(static_cast<_Args&&>(__args)...) {}
  template <std::size_t _Ip, class... _Args>
    requires(_Ip > 0)
  constexpr explicit __var_union(std::in_place_index_t<_Ip>, _Args&&... __args)
      : __tail(std::in_place_index<_Ip - 1>, static_cast<_Args&&>(__args)...) {}
  __var_union(const __var_union&) = default;
  __var_union(__var_union&&) = default;
  __var_union& operator=(const __var_union&) = default;
  __var_union& operator=(__var_union&&) = default;
  constexpr ~__var_union() {}
};

template <std::size_t _Ip, class _Up>
constexpr auto&& __var_raw_get(_Up&& __u) noexcept {
  if constexpr (_Ip == 0)
    return static_cast<_Up&&>(__u).__head;
  else
    return ::__ycxx::__detail::__var_raw_get<_Ip - 1>(static_cast<_Up&&>(__u).__tail);
}

// Indices 0..N-1 plus one value for valueless, so N alternatives fit in a type with N+1 values.
template <std::size_t _Np>
using __var_index_t = std::conditional_t<(_Np < 256), unsigned char, std::conditional_t<(_Np < 65536), unsigned short, unsigned>>;

// ---- converting-constructor alternative selection ([variant.ctor]/14) -----------------------
template <class _Ti>
void __array_init(_Ti (&&)[1]);
template <class _Ti, class _Tp>
concept __no_narrowing_into = requires(_Tp&& t) { __array_init<_Ti>({static_cast<_Tp&&>(t)}); };

// The array declaration is tested for a non-class Ti only. A class (or union) element is
// copy-initialized from t with no narrowing check (narrowing is between arithmetic types), so the
// declaration is valid exactly when t converts implicitly to Ti, or by brace elision into an
// aggregate Ti to which t does not convert, when FUN(Ti) is not viable: FUN's overload resolution
// decides both. Testing it would also make Clang instantiate the constexpr constructor of Ti that
// the conversion names (a braced-init-list's elements are potentially constant evaluated), whose
// body need not be valid for T.
template <std::size_t _Ip, class _Ti>
struct __var_fun {
  template <class _Tp>
    requires(std::is_class_v<_Ti> || std::is_union_v<_Ti> || __no_narrowing_into<_Ti, _Tp>)
  static std::integral_constant<std::size_t, _Ip> fun(_Ti);
};
template <class _Seq, class... _Ts>
struct __var_funs;
template <std::size_t... _Ip, class... _Ts>
struct __var_funs<std::index_sequence<_Ip...>, _Ts...> : __var_fun<_Ip, _Ts>... {
  using __var_fun<_Ip, _Ts>::fun...;
};
template <class _Tp, class... _Ts>
using __var_selected = decltype(__var_funs<std::index_sequence_for<_Ts...>, _Ts...>::template fun<_Tp>(std::declval<_Tp>()));
template <class _Tp, class _Void, class... _Ts>
struct __var_select_impl {};
template <class _Tp, class... _Ts>
struct __var_select_impl<_Tp, std::void_t<__var_selected<_Tp, _Ts...>>, _Ts...> {
  using type = __var_selected<_Tp, _Ts...>;
};
template <class _Tp, class... _Ts>
struct __var_select : __var_select_impl<_Tp, void, _Ts...> {};

// [variant.ctor]/15.2, then /15.4 (is_constructible_v<Tj, T>), as a class that the converting
// constructor names in a default template argument. Tj's constructor from T can need that
// constructor again (Tj constructible from anything, T convertible to the variant:
// llvm.org/PR151328). The nested use then names this class while it is being instantiated, a
// substitution failure that drops the nested candidate; in a requires-clause the constraint's
// satisfaction would depend on itself, which is ill-formed.
template <class _Vp, class _Tj, class _Tp, bool = std::is_same_v<std::remove_cvref_t<_Tp>, _Vp>>
struct __var_accepts : std::bool_constant<false> {};
template <class _Vp, class _Tj, class _Tp>
struct __var_accepts<_Vp, _Tj, _Tp, false> : std::is_constructible<_Tj, _Tp> {};

template <class _Tp, class... _Ts>
consteval std::size_t __count_of() {
  return (std::size_t(0) + ... + std::size_t(std::is_same_v<_Tp, _Ts>));
}
template <class _Tp, class... _Ts>
consteval std::size_t __index_of() {
  constexpr bool __hits[] = {std::is_same_v<_Tp, _Ts>..., false};
  for (std::size_t i = 0; i < sizeof...(_Ts); ++i)
    if (__hits[i])
      return i;
  return sizeof...(_Ts);
}

template <class _Tp>
inline constexpr bool __is_in_place_tag = __is_in_place_type<_Tp> || __is_in_place_index<_Tp>;

template <class... _Ts>
concept __all_trivially_destructible = (std::is_trivially_destructible_v<_Ts> && ...);

// The one way to reach a variant's storage from outside the class.
struct __variant_access {
  template <std::size_t _Ip, class _Vp>
  static constexpr auto&& __raw(_Vp&& __v) noexcept {
    return ::__ycxx::__detail::__var_raw_get<_Ip>(static_cast<_Vp&&>(__v).__u_);
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class... _Types>
class variant {
  static_assert(sizeof...(_Types) > 0, "variant must have at least one alternative");
  static_assert(((!is_array_v<_Types> && !is_reference_v<_Types> && !is_void_v<_Types>) && ...),
                "variant alternatives must be non-array, non-reference, non-void object types");

  static constexpr size_t _Np = sizeof...(_Types);
  using index_type = __ycxx::__detail::__var_index_t<_Np>;
  static constexpr index_type __npos_index = static_cast<index_type>(-1);
  using __storage = __ycxx::__detail::__var_union<__ycxx::__detail::__all_trivially_destructible<_Types...>, _Types...>;

  __storage __u_;
  index_type __index_;

  friend __ycxx::__detail::__variant_access;

  static constexpr bool __trivial_copy = (is_trivially_copy_constructible_v<_Types> && ...);
  static constexpr bool __trivial_move = (is_trivially_move_constructible_v<_Types> && ...);
  static constexpr bool __trivial_copy_assign =
      ((is_trivially_copy_constructible_v<_Types> && is_trivially_copy_assignable_v<_Types> &&
        is_trivially_destructible_v<_Types>) &&
       ...);
  static constexpr bool __trivial_move_assign =
      ((is_trivially_move_constructible_v<_Types> && is_trivially_move_assignable_v<_Types> &&
        is_trivially_destructible_v<_Types>) &&
       ...);

  constexpr void destroy() noexcept {
    if constexpr (!__ycxx::__detail::__all_trivially_destructible<_Types...>) {
      if (__index_ != __npos_index)
        __ycxx::__detail::__dispatch_index<_Np>(__index_, [this](auto i) {
          std::destroy_at(__builtin_addressof(::__ycxx::__detail::__var_raw_get<i>(__u_)));
        });
    }
    __index_ = __npos_index;
  }
  template <size_t _Ip, class... _Args>
  constexpr void construct(_Args&&... __args) {
    // Precondition: no alternative is active (destroyed or valueless). construct_at replaces the
    // whole union; if the alternative's constructor throws, the guard re-creates a (valueless)
    // union so ~variant never runs ~var_union on an object whose lifetime has ended.
    struct __restore_union {
      __storage* __u;
      bool __armed = true;
      constexpr ~__restore_union() {
        if (__armed)
          std::construct_at(__u, __ycxx::__detail::__valueless_tag{});
      }
    } __guard{__builtin_addressof(__u_)};
    std::construct_at(__builtin_addressof(__u_), in_place_index<_Ip>, static_cast<_Args&&>(__args)...);
    __guard.__armed = false;
    __index_ = static_cast<index_type>(_Ip);
  }
  template <class _Vp>
  constexpr void __construct_from(_Vp&& other) {
    if (other.__index_ != __npos_index)
      __ycxx::__detail::__dispatch_index<_Np>(other.__index_, [&](auto i) {
        construct<i>(::__ycxx::__detail::__var_raw_get<i>(static_cast<_Vp&&>(other).__u_));
      });
  }

  // [variant.mod]/7: the contained value is direct-initialized from the arguments themselves.
  // A class-type alternative is never built in a temporary first, not even a trivially copyable
  // one: the move out of the temporary could select another constructor (a template taking
  // U&&), and the stored object would not be the one the constructor ran on. A throwing
  // constructor leaves the variant valueless (/11). Only for a scalar alternative, which has no
  // constructors, is the value computed first (a throwing conversion operator then leaves the old
  // alternative in place); the copy is unobservable.
  template <size_t _Ip, class... _Args>
  constexpr variant_alternative_t<_Ip, variant>& __emplace_impl(_Args&&... __args) {
    using _Ti = _Types...[_Ip];
    if constexpr (is_scalar_v<_Ti> && !is_nothrow_constructible_v<_Ti, _Args...>) {
      _Ti __tmp(static_cast<_Args&&>(__args)...);
      destroy();
      construct<_Ip>(static_cast<_Ti&&>(__tmp));
    } else {
      destroy();
      construct<_Ip>(static_cast<_Args&&>(__args)...);
    }
    return ::__ycxx::__detail::__var_raw_get<_Ip>(__u_);
  }

public:
  // ---- [variant.ctor] ----
  constexpr variant() noexcept(is_nothrow_default_constructible_v<_Types...[0]>)
    requires is_default_constructible_v<_Types...[0]>
      : __u_(in_place_index<0>), __index_(0) {}

  constexpr variant(const variant&)
    requires((is_copy_constructible_v<_Types> && ...) && __trivial_copy)
  = default;
  constexpr variant(const variant& __w) noexcept((is_nothrow_copy_constructible_v<_Types> && ...))
    requires((is_copy_constructible_v<_Types> && ...) && !__trivial_copy)
      : __u_(__ycxx::__detail::__valueless_tag{}), __index_(__npos_index) {
    __construct_from(__w);
  }
  // "Defined as deleted unless ...": an explicitly deleted overload keeps the class trivially
  // copyable on Clang when copying is unavailable.
  constexpr variant(const variant&)
    requires(!(is_copy_constructible_v<_Types> && ...))
  = delete;
  constexpr variant(variant&&)
    requires((is_move_constructible_v<_Types> && ...) && __trivial_move)
  = default;
  constexpr variant(variant&& __w) noexcept((is_nothrow_move_constructible_v<_Types> && ...))
    requires((is_move_constructible_v<_Types> && ...) && !__trivial_move)
      : __u_(__ycxx::__detail::__valueless_tag{}), __index_(__npos_index) {
    __construct_from(static_cast<variant&&>(__w));
  }

  template <class _Tp, class _Jp = typename __ycxx::__detail::__var_select<_Tp, _Types...>::type,
            class = enable_if_t<__ycxx::__detail::__var_accepts<variant, _Types...[_Jp::value], _Tp>::value>>
    requires(!__ycxx::__detail::__is_in_place_tag<remove_cvref_t<_Tp>>)
  constexpr variant(_Tp&& t) noexcept(is_nothrow_constructible_v<_Types...[_Jp::value], _Tp>)
      : __u_(in_place_index<_Jp::value>, static_cast<_Tp&&>(t)), __index_(_Jp::value) {}

  template <class _Tp, class... _Args>
    requires(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1) && is_constructible_v<_Tp, _Args...>
  constexpr explicit variant(in_place_type_t<_Tp>, _Args&&... __args)
      : __u_(in_place_index<__ycxx::__detail::__index_of<_Tp, _Types...>()>, static_cast<_Args&&>(__args)...),
        __index_(__ycxx::__detail::__index_of<_Tp, _Types...>()) {}
  template <class _Tp, class _Up, class... _Args>
    requires(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1) && is_constructible_v<_Tp, initializer_list<_Up>&, _Args...>
  constexpr explicit variant(in_place_type_t<_Tp>, initializer_list<_Up> il, _Args&&... __args)
      : __u_(in_place_index<__ycxx::__detail::__index_of<_Tp, _Types...>()>, il, static_cast<_Args&&>(__args)...),
        __index_(__ycxx::__detail::__index_of<_Tp, _Types...>()) {}
  template <size_t _Ip, class... _Args>
    requires(_Ip < _Np) && is_constructible_v<_Types...[_Ip], _Args...>
  constexpr explicit variant(in_place_index_t<_Ip>, _Args&&... __args)
      : __u_(in_place_index<_Ip>, static_cast<_Args&&>(__args)...), __index_(_Ip) {}
  template <size_t _Ip, class _Up, class... _Args>
    requires(_Ip < _Np) && is_constructible_v<_Types...[_Ip], initializer_list<_Up>&, _Args...>
  constexpr explicit variant(in_place_index_t<_Ip>, initializer_list<_Up> il, _Args&&... __args)
      : __u_(in_place_index<_Ip>, il, static_cast<_Args&&>(__args)...), __index_(_Ip) {}

  // ---- [variant.dtor] ----
  constexpr ~variant()
    requires __ycxx::__detail::__all_trivially_destructible<_Types...>
  = default;
  constexpr ~variant() { destroy(); }

  // ---- [variant.assign] ----
  constexpr variant& operator=(const variant&)
    requires((is_copy_constructible_v<_Types> && is_copy_assignable_v<_Types>) && ...) && __trivial_copy_assign
  = default;
  constexpr variant& operator=(const variant& __rhs)
    requires((is_copy_constructible_v<_Types> && is_copy_assignable_v<_Types>) && ...) && (!__trivial_copy_assign)
  {
    if (__rhs.__index_ == __npos_index) {
      destroy();
    } else if (__index_ == __rhs.__index_) {
      __ycxx::__detail::__dispatch_index<_Np>(__index_, [&](auto i) {
        ::__ycxx::__detail::__var_raw_get<i>(__u_) = ::__ycxx::__detail::__var_raw_get<i>(__rhs.__u_);
      });
    } else {
      __ycxx::__detail::__dispatch_index<_Np>(__rhs.__index_, [&](auto __j) {
        using _Tj = _Types...[__j];
        if constexpr (is_nothrow_copy_constructible_v<_Tj> || !is_nothrow_move_constructible_v<_Tj>)
          this->emplace<__j>(::__ycxx::__detail::__var_raw_get<__j>(__rhs.__u_));
        else
        {
          // [variant.assign]/2.5 says operator=(variant(rhs)); doing the move directly avoids
          // recursing into this function when variant's move assignment is constrained out.
          variant __tmp(__rhs);
          this->emplace<__j>(static_cast<_Tj&&>(::__ycxx::__detail::__var_raw_get<__j>(__tmp.__u_)));
        }
      });
    }
    return *this;
  }
  constexpr variant& operator=(const variant&)
    requires(!((is_copy_constructible_v<_Types> && is_copy_assignable_v<_Types>) && ...))
  = delete;
  constexpr variant& operator=(variant&&)
    requires((is_move_constructible_v<_Types> && is_move_assignable_v<_Types>) && ...) && __trivial_move_assign
  = default;
  constexpr variant& operator=(variant&& __rhs) noexcept(((is_nothrow_move_constructible_v<_Types> &&
                                                          is_nothrow_move_assignable_v<_Types>) &&
                                                         ...))
    requires((is_move_constructible_v<_Types> && is_move_assignable_v<_Types>) && ...) && (!__trivial_move_assign)
  {
    if (__rhs.__index_ == __npos_index) {
      destroy();
    } else if (__index_ == __rhs.__index_) {
      __ycxx::__detail::__dispatch_index<_Np>(__index_, [&](auto i) {
        ::__ycxx::__detail::__var_raw_get<i>(__u_) = static_cast<_Types...[i]&&>(::__ycxx::__detail::__var_raw_get<i>(__rhs.__u_));
      });
    } else {
      __ycxx::__detail::__dispatch_index<_Np>(__rhs.__index_, [&](auto __j) {
        this->emplace<__j>(static_cast<_Types...[__j]&&>(::__ycxx::__detail::__var_raw_get<__j>(__rhs.__u_)));
      });
    }
    return *this;
  }

  template <class _Tp, class _Jp = __ycxx::__detail::__var_selected<_Tp, _Types...>>
    requires(!is_same_v<remove_cvref_t<_Tp>, variant>) && is_assignable_v<_Types...[_Jp::value]&, _Tp> &&
            is_constructible_v<_Types...[_Jp::value], _Tp>
  constexpr variant& operator=(_Tp&& t) noexcept(is_nothrow_assignable_v<_Types...[_Jp::value]&, _Tp> &&
                                               is_nothrow_constructible_v<_Types...[_Jp::value], _Tp>) {
    constexpr size_t __j = _Jp::value;
    using _Tj = _Types...[__j];
    if (__index_ == __j)
      ::__ycxx::__detail::__var_raw_get<__j>(__u_) = static_cast<_Tp&&>(t);
    else if constexpr (is_nothrow_constructible_v<_Tj, _Tp> || !is_nothrow_move_constructible_v<_Tj>)
      emplace<__j>(static_cast<_Tp&&>(t));
    else
      emplace<__j>(_Tj(static_cast<_Tp&&>(t)));
    return *this;
  }

  // ---- [variant.mod] ----
  template <class _Tp, class... _Args>
    requires(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1) && is_constructible_v<_Tp, _Args...>
  constexpr _Tp& emplace(_Args&&... __args) {
    return emplace<__ycxx::__detail::__index_of<_Tp, _Types...>()>(static_cast<_Args&&>(__args)...);
  }
  template <class _Tp, class _Up, class... _Args>
    requires(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1) && is_constructible_v<_Tp, initializer_list<_Up>&, _Args...>
  constexpr _Tp& emplace(initializer_list<_Up> il, _Args&&... __args) {
    return emplace<__ycxx::__detail::__index_of<_Tp, _Types...>()>(il, static_cast<_Args&&>(__args)...);
  }
  template <size_t _Ip, class... _Args>
    requires(_Ip < _Np) && is_constructible_v<_Types...[_Ip], _Args...>
  constexpr variant_alternative_t<_Ip, variant>& emplace(_Args&&... __args) {
    return __emplace_impl<_Ip>(static_cast<_Args&&>(__args)...);
  }

  template <size_t _Ip, class _Up, class... _Args>
    requires(_Ip < _Np) && is_constructible_v<_Types...[_Ip], initializer_list<_Up>&, _Args...>
  constexpr variant_alternative_t<_Ip, variant>& emplace(initializer_list<_Up> il, _Args&&... __args) {
    return __emplace_impl<_Ip>(il, static_cast<_Args&&>(__args)...);
  }

  // ---- [variant.status] ----
  constexpr bool valueless_by_exception() const noexcept { return __index_ == __npos_index; }
  constexpr size_t index() const noexcept { return __index_ == __npos_index ? variant_npos : __index_; }

  // ---- [variant.swap] ----
  constexpr void swap(variant& __rhs) noexcept(((is_nothrow_move_constructible_v<_Types> &&
                                                is_nothrow_swappable_v<_Types>) &&
                                               ...)) {
    static_assert((is_move_constructible_v<_Types> && ...), "variant::swap: alternatives must be move constructible");
    if (__index_ == __npos_index && __rhs.__index_ == __npos_index)
      return;
    if (__index_ == __rhs.__index_) {
      __ycxx::__detail::__dispatch_index<_Np>(__index_, [&](auto i) {
        __ycxx::__detail::__swap_adl::__do_swap(::__ycxx::__detail::__var_raw_get<i>(__u_), ::__ycxx::__detail::__var_raw_get<i>(__rhs.__u_));
      });
      return;
    }
    variant __tmp(static_cast<variant&&>(__rhs));
    __rhs.destroy();
    __rhs.__construct_from(static_cast<variant&&>(*this));
    destroy();
    __construct_from(static_cast<variant&&>(__tmp));
  }

  // ---- [variant.visit] member forms ----
  template <int = 0, class _Self, class _Visitor>
  constexpr decltype(auto) visit(this _Self&& __self, _Visitor&& __vis);
  template <class _Rp, class _Self, class _Visitor>
  constexpr _Rp visit(this _Self&& __self, _Visitor&& __vis);
};

// ---- [variant.get] ----

template <class _Tp, class... _Types>
constexpr bool holds_alternative(const variant<_Types...>& __v) noexcept {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "holds_alternative: T must occur exactly once");
  return __v.index() == __ycxx::__detail::__index_of<_Tp, _Types...>();
}

template <size_t _Ip, class... _Types>
constexpr variant_alternative_t<_Ip, variant<_Types...>>& get(variant<_Types...>& __v) {
  static_assert(_Ip < sizeof...(_Types), "std::get: variant index out of range");
  if (__v.index() != _Ip)
    __ycxx::__detail::__throw_bad_variant_access();
  return __ycxx::__detail::__variant_access::__raw<_Ip>(__v);
}
template <size_t _Ip, class... _Types>
constexpr variant_alternative_t<_Ip, variant<_Types...>>&& get(variant<_Types...>&& __v) {
  static_assert(_Ip < sizeof...(_Types), "std::get: variant index out of range");
  if (__v.index() != _Ip)
    __ycxx::__detail::__throw_bad_variant_access();
  return static_cast<variant_alternative_t<_Ip, variant<_Types...>>&&>(__ycxx::__detail::__variant_access::__raw<_Ip>(__v));
}
template <size_t _Ip, class... _Types>
constexpr const variant_alternative_t<_Ip, variant<_Types...>>& get(const variant<_Types...>& __v) {
  static_assert(_Ip < sizeof...(_Types), "std::get: variant index out of range");
  if (__v.index() != _Ip)
    __ycxx::__detail::__throw_bad_variant_access();
  return __ycxx::__detail::__variant_access::__raw<_Ip>(__v);
}
template <size_t _Ip, class... _Types>
constexpr const variant_alternative_t<_Ip, variant<_Types...>>&& get(const variant<_Types...>&& __v) {
  static_assert(_Ip < sizeof...(_Types), "std::get: variant index out of range");
  if (__v.index() != _Ip)
    __ycxx::__detail::__throw_bad_variant_access();
  return static_cast<const variant_alternative_t<_Ip, variant<_Types...>>&&>(__ycxx::__detail::__variant_access::__raw<_Ip>(__v));
}

template <class _Tp, class... _Types>
constexpr _Tp& get(variant<_Types...>& __v) {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<__ycxx::__detail::__index_of<_Tp, _Types...>()>(__v);
}
template <class _Tp, class... _Types>
constexpr _Tp&& get(variant<_Types...>&& __v) {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<__ycxx::__detail::__index_of<_Tp, _Types...>()>(static_cast<variant<_Types...>&&>(__v));
}
template <class _Tp, class... _Types>
constexpr const _Tp& get(const variant<_Types...>& __v) {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<__ycxx::__detail::__index_of<_Tp, _Types...>()>(__v);
}
template <class _Tp, class... _Types>
constexpr const _Tp&& get(const variant<_Types...>&& __v) {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<__ycxx::__detail::__index_of<_Tp, _Types...>()>(static_cast<const variant<_Types...>&&>(__v));
}

template <size_t _Ip, class... _Types>
constexpr add_pointer_t<variant_alternative_t<_Ip, variant<_Types...>>> get_if(variant<_Types...>* __v) noexcept {
  static_assert(_Ip < sizeof...(_Types), "std::get_if: variant index out of range");
  return __v && __v->index() == _Ip ? __builtin_addressof(__ycxx::__detail::__variant_access::__raw<_Ip>(*__v)) : nullptr;
}
template <size_t _Ip, class... _Types>
constexpr add_pointer_t<const variant_alternative_t<_Ip, variant<_Types...>>> get_if(
    const variant<_Types...>* __v) noexcept {
  static_assert(_Ip < sizeof...(_Types), "std::get_if: variant index out of range");
  return __v && __v->index() == _Ip ? __builtin_addressof(__ycxx::__detail::__variant_access::__raw<_Ip>(*__v)) : nullptr;
}
template <class _Tp, class... _Types>
constexpr add_pointer_t<_Tp> get_if(variant<_Types...>* __v) noexcept {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "std::get_if<T>: T must occur exactly once");
  return std::get_if<__ycxx::__detail::__index_of<_Tp, _Types...>()>(__v);
}
template <class _Tp, class... _Types>
constexpr add_pointer_t<const _Tp> get_if(const variant<_Types...>* __v) noexcept {
  static_assert(__ycxx::__detail::__count_of<_Tp, _Types...>() == 1, "std::get_if<T>: T must occur exactly once");
  return std::get_if<__ycxx::__detail::__index_of<_Tp, _Types...>()>(__v);
}

}} // namespace std

// =============================================================================================
// [variant.visit]
// =============================================================================================
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class... _Ts>
constexpr auto&& __as_variant(std::variant<_Ts...>& __v) noexcept {
  return __v;
}
template <class... _Ts>
constexpr auto&& __as_variant(const std::variant<_Ts...>& __v) noexcept {
  return __v;
}
template <class... _Ts>
constexpr auto&& __as_variant(std::variant<_Ts...>&& __v) noexcept {
  return static_cast<std::variant<_Ts...>&&>(__v);
}
template <class... _Ts>
constexpr auto&& __as_variant(const std::variant<_Ts...>&& __v) noexcept {
  return static_cast<const std::variant<_Ts...>&&>(__v);
}
template <class _Vp>
using __as_variant_t = decltype(::__ycxx::__detail::__as_variant(std::declval<_Vp>()));

// GET<m>(v) on an already-validated index.
template <std::size_t _Ip, class _Vp>
constexpr decltype(auto) __var_unchecked_get(_Vp&& __v) noexcept {
  using _Alt = std::variant_alternative_t<_Ip, std::remove_cvref_t<_Vp>>;
  using _Ref = __copy_cvref<_Vp&&, _Alt>;
  return static_cast<_Ref>(__ycxx::__detail::__variant_access::__raw<_Ip>(__v));
}

// Result type of the visitor for the all-zero index pack (the Mandates check compares every
// other combination against it).
template <class _Vis, class... _Vp>
using __visit_result_t = decltype(::__ycxx::__detail::invoke(std::declval<_Vis>(), ::__ycxx::__detail::__var_unchecked_get<0>(std::declval<_Vp>())...));

template <class _Rp, bool _Exact, class _Vis>
constexpr _Rp __visit_finish(_Vis&& __vis, auto&&... __alts) {
  // [variant.visit]/5 Mandates: one type and value category for every combination.
  static_assert(!_Exact || std::is_same_v<decltype(::__ycxx::__detail::invoke(static_cast<_Vis&&>(__vis),
                                                                         static_cast<decltype(__alts)&&>(__alts)...)),
                                         _Rp>,
                "std::visit: the visitor must return the same type and value category for all alternatives");
  if constexpr (std::is_void_v<_Rp>)
    static_cast<void>(::__ycxx::__detail::invoke(static_cast<_Vis&&>(__vis), static_cast<decltype(__alts)&&>(__alts)...));
  else if constexpr (_Exact) {
    return ::__ycxx::__detail::invoke(static_cast<_Vis&&>(__vis), static_cast<decltype(__alts)&&>(__alts)...);
  } else
    return ::__ycxx::__detail::invoke_r<_Rp>(static_cast<_Vis&&>(__vis), static_cast<decltype(__alts)&&>(__alts)...);
}

// Bind alternatives left to right. `__alts` are the already-selected alternatives.
template <class _Rp, bool _Exact, class _Vis, class _Vp, class... _Vs>
constexpr _Rp __visit_bind(_Vis&& __vis, _Vp&& __v, _Vs&&... __vs) {
  constexpr std::size_t n = std::variant_size_v<std::remove_cvref_t<_Vp>>;
  return ::__ycxx::__detail::__dispatch_index<n>(__v.index(), [&](auto i) -> _Rp {
    decltype(auto) __alt = ::__ycxx::__detail::__var_unchecked_get<i>(static_cast<_Vp&&>(__v));
    if constexpr (sizeof...(_Vs) == 0) {
      return ::__ycxx::__detail::__visit_finish<_Rp, _Exact>(static_cast<_Vis&&>(__vis), static_cast<decltype(__alt)&&>(__alt));
    } else {
      // Curry: a visitor that receives the remaining alternatives and prepends `__alt`.
      auto __curried = [&](auto&&... __rest) -> _Rp {
        return ::__ycxx::__detail::__visit_finish<_Rp, _Exact>(static_cast<_Vis&&>(__vis), static_cast<decltype(__alt)&&>(__alt),
                                      static_cast<decltype(__rest)&&>(__rest)...);
      };
      return ::__ycxx::__detail::__visit_bind<_Rp, true>(__curried, static_cast<_Vs&&>(__vs)...);
    }
  });
}

template <bool _Exact, class _Rp, class _Vis, class... _Vp>
constexpr _Rp __visit_entry(_Vis&& __vis, _Vp&&... __vars) {
  if ((__vars.valueless_by_exception() || ...))
    __throw_bad_variant_access();
  if constexpr (sizeof...(_Vp) == 0)
    return ::__ycxx::__detail::__visit_finish<_Rp, _Exact>(static_cast<_Vis&&>(__vis));
  else
    return ::__ycxx::__detail::__visit_bind<_Rp, _Exact>(static_cast<_Vis&&>(__vis), static_cast<_Vp&&>(__vars)...);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Visitor, class... _Variants>
  requires(requires { typename __ycxx::__detail::__as_variant_t<_Variants>; } && ...)
constexpr decltype(auto) visit(_Visitor&& __vis, _Variants&&... __vars) {
  // The result type is computed in the body, not the signature: [variant.visit]/5 makes a
  // visitor that is not callable with every alternative a Mandates violation (a hard error),
  // not a reason to drop out of overload resolution.
  using _Rp = __ycxx::__detail::__visit_result_t<_Visitor, __ycxx::__detail::__as_variant_t<_Variants>...>;
  return __ycxx::__detail::__visit_entry<true, _Rp>(static_cast<_Visitor&&>(__vis),
                                            __ycxx::__detail::__as_variant(static_cast<_Variants&&>(__vars))...);
}
template <class _Rp, class _Visitor, class... _Variants>
  requires(requires { typename __ycxx::__detail::__as_variant_t<_Variants>; } && ...)
constexpr _Rp visit(_Visitor&& __vis, _Variants&&... __vars) {
  return __ycxx::__detail::__visit_entry<false, _Rp>(static_cast<_Visitor&&>(__vis),
                                             __ycxx::__detail::__as_variant(static_cast<_Variants&&>(__vars))...);
}

template <class... _Types>
template <int, class _Self, class _Visitor>
constexpr decltype(auto) variant<_Types...>::visit(this _Self&& __self, _Visitor&& __vis) {
  using _Vp = __ycxx::__detail::__copy_cvref<_Self&&, variant>;
  // [variant.visit]/9 specifies (V)self: a C-style cast reaches an inaccessible (private) base.
  return std::visit(static_cast<_Visitor&&>(__vis), (_Vp)__self);
}
template <class... _Types>
template <class _Rp, class _Self, class _Visitor>
constexpr _Rp variant<_Types...>::visit(this _Self&& __self, _Visitor&& __vis) {
  using _Vp = __ycxx::__detail::__copy_cvref<_Self&&, variant>;
  return std::visit<_Rp>(static_cast<_Visitor&&>(__vis), (_Vp)__self);
}

// ---- [variant.relops] ----

template <class... _Types>
  requires((requires(const _Types& a) {
    { a == a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator==(const variant<_Types...>& __v, const variant<_Types...>& __w) {
  if (__v.index() != __w.index())
    return false;
  if (__v.valueless_by_exception())
    return true;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> bool {
    return static_cast<bool>(__ycxx::__detail::__variant_access::__raw<i>(__v) == __ycxx::__detail::__variant_access::__raw<i>(__w));
  });
}
template <class... _Types>
  requires((requires(const _Types& a) {
    { a != a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator!=(const variant<_Types...>& __v, const variant<_Types...>& __w) {
  if (__v.index() != __w.index())
    return true;
  if (__v.valueless_by_exception())
    return false;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> bool {
    return static_cast<bool>(__ycxx::__detail::__variant_access::__raw<i>(__v) != __ycxx::__detail::__variant_access::__raw<i>(__w));
  });
}
template <class... _Types>
  requires((requires(const _Types& a) {
    { a < a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator<(const variant<_Types...>& __v, const variant<_Types...>& __w) {
  if (__w.valueless_by_exception())
    return false;
  if (__v.valueless_by_exception())
    return true;
  if (__v.index() < __w.index())
    return true;
  if (__v.index() > __w.index())
    return false;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> bool {
    return static_cast<bool>(__ycxx::__detail::__variant_access::__raw<i>(__v) < __ycxx::__detail::__variant_access::__raw<i>(__w));
  });
}
template <class... _Types>
  requires((requires(const _Types& a) {
    { a > a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator>(const variant<_Types...>& __v, const variant<_Types...>& __w) {
  if (__v.valueless_by_exception())
    return false;
  if (__w.valueless_by_exception())
    return true;
  if (__v.index() > __w.index())
    return true;
  if (__v.index() < __w.index())
    return false;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> bool {
    return static_cast<bool>(__ycxx::__detail::__variant_access::__raw<i>(__v) > __ycxx::__detail::__variant_access::__raw<i>(__w));
  });
}
template <class... _Types>
  requires((requires(const _Types& a) {
    { a <= a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator<=(const variant<_Types...>& __v, const variant<_Types...>& __w) {
  if (__v.valueless_by_exception())
    return true;
  if (__w.valueless_by_exception())
    return false;
  if (__v.index() < __w.index())
    return true;
  if (__v.index() > __w.index())
    return false;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> bool {
    return static_cast<bool>(__ycxx::__detail::__variant_access::__raw<i>(__v) <= __ycxx::__detail::__variant_access::__raw<i>(__w));
  });
}
template <class... _Types>
  requires((requires(const _Types& a) {
    { a >= a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator>=(const variant<_Types...>& __v, const variant<_Types...>& __w) {
  if (__w.valueless_by_exception())
    return true;
  if (__v.valueless_by_exception())
    return false;
  if (__v.index() > __w.index())
    return true;
  if (__v.index() < __w.index())
    return false;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> bool {
    return static_cast<bool>(__ycxx::__detail::__variant_access::__raw<i>(__v) >= __ycxx::__detail::__variant_access::__raw<i>(__w));
  });
}
template <class... _Types>
  requires(three_way_comparable<_Types> && ...)
constexpr common_comparison_category_t<compare_three_way_result_t<_Types>...> operator<=>(const variant<_Types...>& __v,
                                                                                        const variant<_Types...>& __w) {
  using _Rp = common_comparison_category_t<compare_three_way_result_t<_Types>...>;
  if (__v.valueless_by_exception() && __w.valueless_by_exception())
    return strong_ordering::equal;
  if (__v.valueless_by_exception())
    return strong_ordering::less;
  if (__w.valueless_by_exception())
    return strong_ordering::greater;
  if (auto c = __v.index() <=> __w.index(); c != 0)
    return c;
  return __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> _Rp {
    return __ycxx::__detail::__variant_access::__raw<i>(__v) <=> __ycxx::__detail::__variant_access::__raw<i>(__w);
  });
}

// ---- [variant.specalg] ----
template <class... _Types>
  requires((is_move_constructible_v<_Types> && is_swappable_v<_Types>) && ...)
constexpr void swap(variant<_Types...>& __v, variant<_Types...>& __w) noexcept(noexcept(__v.swap(__w))) {
  __v.swap(__w);
}

// ---- [variant.hash] ----
template <class... _Types>
  requires(__ycxx::__detail::__hash_enabled<remove_const_t<_Types>> && ...)
struct hash<variant<_Types...>> {
  size_t operator()(const variant<_Types...>& __v) const {
    if (__v.valueless_by_exception())
      return static_cast<size_t>(0x76616c75u);
    size_t h = __ycxx::__detail::__dispatch_index<sizeof...(_Types)>(__v.index(), [&](auto i) -> size_t {
      return hash<remove_const_t<_Types...[i]>>{}(__ycxx::__detail::__variant_access::__raw<i>(__v));
    });
    return static_cast<size_t>(__ycxx::__detail::__mum(h ^ __ycxx::__detail::__hash_k1, __v.index() ^ __ycxx::__detail::__hash_k2));
  }
};

}} // namespace std
