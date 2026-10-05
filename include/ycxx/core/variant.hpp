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

namespace std {

class bad_variant_access : public exception {
public:
  constexpr bad_variant_access() noexcept {}
  constexpr bad_variant_access(const bad_variant_access&) noexcept = default;
  constexpr bad_variant_access& operator=(const bad_variant_access&) noexcept = default;
  constexpr ~bad_variant_access() override {}
  constexpr const char* what() const noexcept override { return "bad variant access"; }
};

} // namespace std

namespace ycxx::detail {
[[noreturn]] [[gnu::cold]] constexpr void throw_bad_variant_access() {
  ::ycxx::detail::raise_with(ycxx_error_bad_variant_access, "std::bad_variant_access", [] { return std::bad_variant_access(); });
}
} // namespace ycxx::detail

namespace std {

template <class... Types>
class variant;

template <class T>
struct variant_size;
template <class T>
struct variant_size<const T> : variant_size<T> {};
// [depr.variant] (Annex D); the members repeat the attribute for GCC (see tuple_like.hpp).
template <class T>
struct [[deprecated("variant_size<volatile T> is deprecated ([depr.variant])")]] variant_size<volatile T>
    : integral_constant<size_t, variant_size<T>::value> {
  [[deprecated("variant_size<volatile T> is deprecated ([depr.variant])")]]
  static constexpr size_t value = variant_size<T>::value;
};
template <class T>
struct [[deprecated("variant_size<const volatile T> is deprecated ([depr.variant])")]] variant_size<const volatile T>
    : integral_constant<size_t, variant_size<T>::value> {
  [[deprecated("variant_size<const volatile T> is deprecated ([depr.variant])")]]
  static constexpr size_t value = variant_size<T>::value;
};
template <class T>
constexpr size_t variant_size_v = variant_size<T>::value;
template <class... Types>
struct variant_size<variant<Types...>> : integral_constant<size_t, sizeof...(Types)> {};

template <size_t I, class T>
struct variant_alternative;
template <size_t I, class T>
struct variant_alternative<I, const T> {
  using type = const typename variant_alternative<I, T>::type;
};
template <size_t I, class T>
struct [[deprecated("variant_alternative<I, volatile T> is deprecated ([depr.variant])")]] variant_alternative<I, volatile T> {
  using type [[deprecated("variant_alternative<I, volatile T> is deprecated ([depr.variant])")]] =
      volatile typename variant_alternative<I, T>::type;
};
template <size_t I, class T>
struct [[deprecated("variant_alternative<I, const volatile T> is deprecated ([depr.variant])")]]
    variant_alternative<I, const volatile T> {
  using type [[deprecated("variant_alternative<I, const volatile T> is deprecated ([depr.variant])")]] =
      const volatile typename variant_alternative<I, T>::type;
};
template <size_t I, class T>
using variant_alternative_t = typename variant_alternative<I, T>::type;
template <size_t I, class... Types>
struct variant_alternative<I, variant<Types...>> {
  static_assert(I < sizeof...(Types), "variant_alternative index out of range");
  using type = Types...[I];
};

inline constexpr size_t variant_npos = static_cast<size_t>(-1);

} // namespace std

namespace ycxx::detail {

// ---- index dispatch ---------------------------------------------------------------------------
// Calls f(integral_constant<size_t, i>{}) for a run-time i < N. Small N uses a compare chain,
// which both compilers inline (GCC keeps the indirect call through a table); larger N uses a
// table of function pointers. Both work in constant evaluation.
inline constexpr std::size_t dispatch_chain_max = 12;

template <std::size_t I, std::size_t N, class R, class F>
constexpr R dispatch_chain(std::size_t i, F&& f) {
  if constexpr (I + 1 == N)
    return static_cast<F&&>(f)(std::integral_constant<std::size_t, I>{});
  else if (i == I)
    return static_cast<F&&>(f)(std::integral_constant<std::size_t, I>{});
  else
    return ::ycxx::detail::dispatch_chain<I + 1, N, R>(i, static_cast<F&&>(f));
}

template <std::size_t N, class F>
constexpr decltype(auto) dispatch_index(std::size_t i, F&& f) {
  using R = decltype(static_cast<F&&>(f)(std::integral_constant<std::size_t, 0>{}));
  if constexpr (N <= dispatch_chain_max)
    return ::ycxx::detail::dispatch_chain<0, N, R>(i, static_cast<F&&>(f));
  else
    return [&]<std::size_t... I>(std::index_sequence<I...>) -> R {
    static constexpr R (*table[])(F&&) = {+[](F&& g) -> R {
      return static_cast<F&&>(g)(std::integral_constant<std::size_t, I>{});
    }...};
      return table[i](static_cast<F&&>(f));
    }(std::make_index_sequence<N>{});
}

// ---- storage ------------------------------------------------------------------------------------
struct valueless_tag {};

template <bool TrivialDtor, class... Ts>
union var_union;

template <bool TD>
union var_union<TD> {
  constexpr var_union(valueless_tag) noexcept {}
};

template <class T, class... Ts>
union var_union<true, T, Ts...> {
  T head;
  var_union<true, Ts...> tail;

  constexpr var_union(valueless_tag) noexcept : tail(valueless_tag{}) {}
  template <class... Args>
  constexpr explicit var_union(std::in_place_index_t<0>, Args&&... args) : head(static_cast<Args&&>(args)...) {}
  template <std::size_t I, class... Args>
    requires(I > 0)
  constexpr explicit var_union(std::in_place_index_t<I>, Args&&... args)
      : tail(std::in_place_index<I - 1>, static_cast<Args&&>(args)...) {}
};

template <class T, class... Ts>
union var_union<false, T, Ts...> {
  T head;
  var_union<false, Ts...> tail;

  constexpr var_union(valueless_tag) noexcept : tail(valueless_tag{}) {}
  template <class... Args>
  constexpr explicit var_union(std::in_place_index_t<0>, Args&&... args) : head(static_cast<Args&&>(args)...) {}
  template <std::size_t I, class... Args>
    requires(I > 0)
  constexpr explicit var_union(std::in_place_index_t<I>, Args&&... args)
      : tail(std::in_place_index<I - 1>, static_cast<Args&&>(args)...) {}
  var_union(const var_union&) = default;
  var_union(var_union&&) = default;
  var_union& operator=(const var_union&) = default;
  var_union& operator=(var_union&&) = default;
  constexpr ~var_union() {}
};

template <std::size_t I, class U>
constexpr auto&& var_raw_get(U&& u) noexcept {
  if constexpr (I == 0)
    return static_cast<U&&>(u).head;
  else
    return ::ycxx::detail::var_raw_get<I - 1>(static_cast<U&&>(u).tail);
}

// Indices 0..N-1 plus one value for valueless, so N alternatives fit in a type with N+1 values.
template <std::size_t N>
using var_index_t = std::conditional_t<(N < 256), unsigned char, std::conditional_t<(N < 65536), unsigned short, unsigned>>;

// ---- converting-constructor alternative selection ([variant.ctor]/14) -----------------------
template <class Ti>
void array_init(Ti (&&)[1]);
template <class Ti, class T>
concept no_narrowing_into = requires(T&& t) { array_init<Ti>({static_cast<T&&>(t)}); };

template <std::size_t I, class Ti>
struct var_fun {
  template <class T>
    requires no_narrowing_into<Ti, T>
  static std::integral_constant<std::size_t, I> fun(Ti);
};
template <class Seq, class... Ts>
struct var_funs;
template <std::size_t... I, class... Ts>
struct var_funs<std::index_sequence<I...>, Ts...> : var_fun<I, Ts>... {
  using var_fun<I, Ts>::fun...;
};
template <class T, class... Ts>
using var_selected = decltype(var_funs<std::index_sequence_for<Ts...>, Ts...>::template fun<T>(std::declval<T>()));

template <class T, class... Ts>
consteval std::size_t count_of() {
  return (std::size_t(0) + ... + std::size_t(std::is_same_v<T, Ts>));
}
template <class T, class... Ts>
consteval std::size_t index_of() {
  constexpr bool hits[] = {std::is_same_v<T, Ts>..., false};
  for (std::size_t i = 0; i < sizeof...(Ts); ++i)
    if (hits[i])
      return i;
  return sizeof...(Ts);
}

template <class T>
inline constexpr bool is_in_place_tag = is_in_place_type<T> || is_in_place_index<T>;

template <class... Ts>
concept all_trivially_destructible = (std::is_trivially_destructible_v<Ts> && ...);

// The one way to reach a variant's storage from outside the class.
struct variant_access {
  template <std::size_t I, class V>
  static constexpr auto&& raw(V&& v) noexcept {
    return ::ycxx::detail::var_raw_get<I>(static_cast<V&&>(v).u_);
  }
};

} // namespace ycxx::detail

namespace std {

template <class... Types>
class variant {
  static_assert(sizeof...(Types) > 0, "variant must have at least one alternative");
  static_assert(((!is_array_v<Types> && !is_reference_v<Types> && !is_void_v<Types>) && ...),
                "variant alternatives must be non-array, non-reference, non-void object types");

  static constexpr size_t N = sizeof...(Types);
  using index_type = ycxx::detail::var_index_t<N>;
  static constexpr index_type npos_index = static_cast<index_type>(-1);
  using storage = ycxx::detail::var_union<ycxx::detail::all_trivially_destructible<Types...>, Types...>;

  storage u_;
  index_type index_;

  friend ycxx::detail::variant_access;

  static constexpr bool trivial_copy = (is_trivially_copy_constructible_v<Types> && ...);
  static constexpr bool trivial_move = (is_trivially_move_constructible_v<Types> && ...);
  static constexpr bool trivial_copy_assign =
      ((is_trivially_copy_constructible_v<Types> && is_trivially_copy_assignable_v<Types> &&
        is_trivially_destructible_v<Types>) &&
       ...);
  static constexpr bool trivial_move_assign =
      ((is_trivially_move_constructible_v<Types> && is_trivially_move_assignable_v<Types> &&
        is_trivially_destructible_v<Types>) &&
       ...);

  constexpr void destroy() noexcept {
    if constexpr (!ycxx::detail::all_trivially_destructible<Types...>) {
      if (index_ != npos_index)
        ycxx::detail::dispatch_index<N>(index_, [this](auto i) {
          std::destroy_at(__builtin_addressof(::ycxx::detail::var_raw_get<i>(u_)));
        });
    }
    index_ = npos_index;
  }
  template <size_t I, class... Args>
  constexpr void construct(Args&&... args) {
    // Precondition: no alternative is active (destroyed or valueless). construct_at replaces the
    // whole union; if the alternative's constructor throws, the guard re-creates a (valueless)
    // union so ~variant never runs ~var_union on an object whose lifetime has ended.
    struct restore_union {
      storage* u;
      bool armed = true;
      constexpr ~restore_union() {
        if (armed)
          std::construct_at(u, ycxx::detail::valueless_tag{});
      }
    } guard{__builtin_addressof(u_)};
    std::construct_at(__builtin_addressof(u_), in_place_index<I>, static_cast<Args&&>(args)...);
    guard.armed = false;
    index_ = static_cast<index_type>(I);
  }
  template <class V>
  constexpr void construct_from(V&& other) {
    if (other.index_ != npos_index)
      ycxx::detail::dispatch_index<N>(other.index_, [&](auto i) {
        construct<i>(::ycxx::detail::var_raw_get<i>(static_cast<V&&>(other).u_));
      });
  }

  // [variant.mod]/7: the contained value is direct-initialized from the arguments themselves.
  // A class-type alternative is never built in a temporary first, not even a trivially copyable
  // one: the move out of the temporary could select another constructor (a template taking
  // U&&), and the stored object would not be the one the constructor ran on. A throwing
  // constructor leaves the variant valueless (/11). Only for a scalar alternative, which has no
  // constructors, is the value computed first (a throwing conversion operator then leaves the old
  // alternative in place); the copy is unobservable.
  template <size_t I, class... Args>
  constexpr variant_alternative_t<I, variant>& emplace_impl(Args&&... args) {
    using Ti = Types...[I];
    if constexpr (is_scalar_v<Ti> && !is_nothrow_constructible_v<Ti, Args...>) {
      Ti tmp(static_cast<Args&&>(args)...);
      destroy();
      construct<I>(static_cast<Ti&&>(tmp));
    } else {
      destroy();
      construct<I>(static_cast<Args&&>(args)...);
    }
    return ::ycxx::detail::var_raw_get<I>(u_);
  }

public:
  // ---- [variant.ctor] ----
  constexpr variant() noexcept(is_nothrow_default_constructible_v<Types...[0]>)
    requires is_default_constructible_v<Types...[0]>
      : u_(in_place_index<0>), index_(0) {}

  constexpr variant(const variant&)
    requires((is_copy_constructible_v<Types> && ...) && trivial_copy)
  = default;
  constexpr variant(const variant& w) noexcept((is_nothrow_copy_constructible_v<Types> && ...))
    requires((is_copy_constructible_v<Types> && ...) && !trivial_copy)
      : u_(ycxx::detail::valueless_tag{}), index_(npos_index) {
    construct_from(w);
  }
  // "Defined as deleted unless ...": an explicitly deleted overload keeps the class trivially
  // copyable on Clang when copying is unavailable.
  constexpr variant(const variant&)
    requires(!(is_copy_constructible_v<Types> && ...))
  = delete;
  constexpr variant(variant&&)
    requires((is_move_constructible_v<Types> && ...) && trivial_move)
  = default;
  constexpr variant(variant&& w) noexcept((is_nothrow_move_constructible_v<Types> && ...))
    requires((is_move_constructible_v<Types> && ...) && !trivial_move)
      : u_(ycxx::detail::valueless_tag{}), index_(npos_index) {
    construct_from(static_cast<variant&&>(w));
  }

  template <class T, class J = ycxx::detail::var_selected<T, Types...>>
    requires(!is_same_v<remove_cvref_t<T>, variant>) && (!ycxx::detail::is_in_place_tag<remove_cvref_t<T>>) &&
            is_constructible_v<Types...[J::value], T>
  constexpr variant(T&& t) noexcept(is_nothrow_constructible_v<Types...[J::value], T>)
      : u_(in_place_index<J::value>, static_cast<T&&>(t)), index_(J::value) {}

  template <class T, class... Args>
    requires(ycxx::detail::count_of<T, Types...>() == 1) && is_constructible_v<T, Args...>
  constexpr explicit variant(in_place_type_t<T>, Args&&... args)
      : u_(in_place_index<ycxx::detail::index_of<T, Types...>()>, static_cast<Args&&>(args)...),
        index_(ycxx::detail::index_of<T, Types...>()) {}
  template <class T, class U, class... Args>
    requires(ycxx::detail::count_of<T, Types...>() == 1) && is_constructible_v<T, initializer_list<U>&, Args...>
  constexpr explicit variant(in_place_type_t<T>, initializer_list<U> il, Args&&... args)
      : u_(in_place_index<ycxx::detail::index_of<T, Types...>()>, il, static_cast<Args&&>(args)...),
        index_(ycxx::detail::index_of<T, Types...>()) {}
  template <size_t I, class... Args>
    requires(I < N) && is_constructible_v<Types...[I], Args...>
  constexpr explicit variant(in_place_index_t<I>, Args&&... args)
      : u_(in_place_index<I>, static_cast<Args&&>(args)...), index_(I) {}
  template <size_t I, class U, class... Args>
    requires(I < N) && is_constructible_v<Types...[I], initializer_list<U>&, Args...>
  constexpr explicit variant(in_place_index_t<I>, initializer_list<U> il, Args&&... args)
      : u_(in_place_index<I>, il, static_cast<Args&&>(args)...), index_(I) {}

  // ---- [variant.dtor] ----
  constexpr ~variant()
    requires ycxx::detail::all_trivially_destructible<Types...>
  = default;
  constexpr ~variant() { destroy(); }

  // ---- [variant.assign] ----
  constexpr variant& operator=(const variant&)
    requires((is_copy_constructible_v<Types> && is_copy_assignable_v<Types>) && ...) && trivial_copy_assign
  = default;
  constexpr variant& operator=(const variant& rhs)
    requires((is_copy_constructible_v<Types> && is_copy_assignable_v<Types>) && ...) && (!trivial_copy_assign)
  {
    if (rhs.index_ == npos_index) {
      destroy();
    } else if (index_ == rhs.index_) {
      ycxx::detail::dispatch_index<N>(index_, [&](auto i) {
        ::ycxx::detail::var_raw_get<i>(u_) = ::ycxx::detail::var_raw_get<i>(rhs.u_);
      });
    } else {
      ycxx::detail::dispatch_index<N>(rhs.index_, [&](auto j) {
        using Tj = Types...[j];
        if constexpr (is_nothrow_copy_constructible_v<Tj> || !is_nothrow_move_constructible_v<Tj>)
          this->emplace<j>(::ycxx::detail::var_raw_get<j>(rhs.u_));
        else
        {
          // [variant.assign]/2.5 says operator=(variant(rhs)); doing the move directly avoids
          // recursing into this function when variant's move assignment is constrained out.
          variant tmp(rhs);
          this->emplace<j>(static_cast<Tj&&>(::ycxx::detail::var_raw_get<j>(tmp.u_)));
        }
      });
    }
    return *this;
  }
  constexpr variant& operator=(const variant&)
    requires(!((is_copy_constructible_v<Types> && is_copy_assignable_v<Types>) && ...))
  = delete;
  constexpr variant& operator=(variant&&)
    requires((is_move_constructible_v<Types> && is_move_assignable_v<Types>) && ...) && trivial_move_assign
  = default;
  constexpr variant& operator=(variant&& rhs) noexcept(((is_nothrow_move_constructible_v<Types> &&
                                                          is_nothrow_move_assignable_v<Types>) &&
                                                         ...))
    requires((is_move_constructible_v<Types> && is_move_assignable_v<Types>) && ...) && (!trivial_move_assign)
  {
    if (rhs.index_ == npos_index) {
      destroy();
    } else if (index_ == rhs.index_) {
      ycxx::detail::dispatch_index<N>(index_, [&](auto i) {
        ::ycxx::detail::var_raw_get<i>(u_) = static_cast<Types...[i]&&>(::ycxx::detail::var_raw_get<i>(rhs.u_));
      });
    } else {
      ycxx::detail::dispatch_index<N>(rhs.index_, [&](auto j) {
        this->emplace<j>(static_cast<Types...[j]&&>(::ycxx::detail::var_raw_get<j>(rhs.u_)));
      });
    }
    return *this;
  }

  template <class T, class J = ycxx::detail::var_selected<T, Types...>>
    requires(!is_same_v<remove_cvref_t<T>, variant>) && is_assignable_v<Types...[J::value]&, T> &&
            is_constructible_v<Types...[J::value], T>
  constexpr variant& operator=(T&& t) noexcept(is_nothrow_assignable_v<Types...[J::value]&, T> &&
                                               is_nothrow_constructible_v<Types...[J::value], T>) {
    constexpr size_t j = J::value;
    using Tj = Types...[j];
    if (index_ == j)
      ::ycxx::detail::var_raw_get<j>(u_) = static_cast<T&&>(t);
    else if constexpr (is_nothrow_constructible_v<Tj, T> || !is_nothrow_move_constructible_v<Tj>)
      emplace<j>(static_cast<T&&>(t));
    else
      emplace<j>(Tj(static_cast<T&&>(t)));
    return *this;
  }

  // ---- [variant.mod] ----
  template <class T, class... Args>
    requires(ycxx::detail::count_of<T, Types...>() == 1) && is_constructible_v<T, Args...>
  constexpr T& emplace(Args&&... args) {
    return emplace<ycxx::detail::index_of<T, Types...>()>(static_cast<Args&&>(args)...);
  }
  template <class T, class U, class... Args>
    requires(ycxx::detail::count_of<T, Types...>() == 1) && is_constructible_v<T, initializer_list<U>&, Args...>
  constexpr T& emplace(initializer_list<U> il, Args&&... args) {
    return emplace<ycxx::detail::index_of<T, Types...>()>(il, static_cast<Args&&>(args)...);
  }
  template <size_t I, class... Args>
    requires(I < N) && is_constructible_v<Types...[I], Args...>
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args) {
    return emplace_impl<I>(static_cast<Args&&>(args)...);
  }

  template <size_t I, class U, class... Args>
    requires(I < N) && is_constructible_v<Types...[I], initializer_list<U>&, Args...>
  constexpr variant_alternative_t<I, variant>& emplace(initializer_list<U> il, Args&&... args) {
    return emplace_impl<I>(il, static_cast<Args&&>(args)...);
  }

  // ---- [variant.status] ----
  constexpr bool valueless_by_exception() const noexcept { return index_ == npos_index; }
  constexpr size_t index() const noexcept { return index_ == npos_index ? variant_npos : index_; }

  // ---- [variant.swap] ----
  constexpr void swap(variant& rhs) noexcept(((is_nothrow_move_constructible_v<Types> &&
                                                is_nothrow_swappable_v<Types>) &&
                                               ...)) {
    static_assert((is_move_constructible_v<Types> && ...), "variant::swap: alternatives must be move constructible");
    if (index_ == npos_index && rhs.index_ == npos_index)
      return;
    if (index_ == rhs.index_) {
      ycxx::detail::dispatch_index<N>(index_, [&](auto i) {
        ycxx::detail::swap_adl::do_swap(::ycxx::detail::var_raw_get<i>(u_), ::ycxx::detail::var_raw_get<i>(rhs.u_));
      });
      return;
    }
    variant tmp(static_cast<variant&&>(rhs));
    rhs.destroy();
    rhs.construct_from(static_cast<variant&&>(*this));
    destroy();
    construct_from(static_cast<variant&&>(tmp));
  }

  // ---- [variant.visit] member forms ----
  template <int = 0, class Self, class Visitor>
  constexpr decltype(auto) visit(this Self&& self, Visitor&& vis);
  template <class R, class Self, class Visitor>
  constexpr R visit(this Self&& self, Visitor&& vis);
};

// ---- [variant.get] ----

template <class T, class... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "holds_alternative: T must occur exactly once");
  return v.index() == ycxx::detail::index_of<T, Types...>();
}

template <size_t I, class... Types>
constexpr variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& v) {
  static_assert(I < sizeof...(Types), "std::get: variant index out of range");
  if (v.index() != I)
    ycxx::detail::throw_bad_variant_access();
  return ycxx::detail::variant_access::raw<I>(v);
}
template <size_t I, class... Types>
constexpr variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& v) {
  static_assert(I < sizeof...(Types), "std::get: variant index out of range");
  if (v.index() != I)
    ycxx::detail::throw_bad_variant_access();
  return static_cast<variant_alternative_t<I, variant<Types...>>&&>(ycxx::detail::variant_access::raw<I>(v));
}
template <size_t I, class... Types>
constexpr const variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& v) {
  static_assert(I < sizeof...(Types), "std::get: variant index out of range");
  if (v.index() != I)
    ycxx::detail::throw_bad_variant_access();
  return ycxx::detail::variant_access::raw<I>(v);
}
template <size_t I, class... Types>
constexpr const variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& v) {
  static_assert(I < sizeof...(Types), "std::get: variant index out of range");
  if (v.index() != I)
    ycxx::detail::throw_bad_variant_access();
  return static_cast<const variant_alternative_t<I, variant<Types...>>&&>(ycxx::detail::variant_access::raw<I>(v));
}

template <class T, class... Types>
constexpr T& get(variant<Types...>& v) {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<ycxx::detail::index_of<T, Types...>()>(v);
}
template <class T, class... Types>
constexpr T&& get(variant<Types...>&& v) {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<ycxx::detail::index_of<T, Types...>()>(static_cast<variant<Types...>&&>(v));
}
template <class T, class... Types>
constexpr const T& get(const variant<Types...>& v) {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<ycxx::detail::index_of<T, Types...>()>(v);
}
template <class T, class... Types>
constexpr const T&& get(const variant<Types...>&& v) {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "std::get<T>: T must occur exactly once");
  return std::get<ycxx::detail::index_of<T, Types...>()>(static_cast<const variant<Types...>&&>(v));
}

template <size_t I, class... Types>
constexpr add_pointer_t<variant_alternative_t<I, variant<Types...>>> get_if(variant<Types...>* v) noexcept {
  static_assert(I < sizeof...(Types), "std::get_if: variant index out of range");
  return v && v->index() == I ? __builtin_addressof(ycxx::detail::variant_access::raw<I>(*v)) : nullptr;
}
template <size_t I, class... Types>
constexpr add_pointer_t<const variant_alternative_t<I, variant<Types...>>> get_if(
    const variant<Types...>* v) noexcept {
  static_assert(I < sizeof...(Types), "std::get_if: variant index out of range");
  return v && v->index() == I ? __builtin_addressof(ycxx::detail::variant_access::raw<I>(*v)) : nullptr;
}
template <class T, class... Types>
constexpr add_pointer_t<T> get_if(variant<Types...>* v) noexcept {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "std::get_if<T>: T must occur exactly once");
  return std::get_if<ycxx::detail::index_of<T, Types...>()>(v);
}
template <class T, class... Types>
constexpr add_pointer_t<const T> get_if(const variant<Types...>* v) noexcept {
  static_assert(ycxx::detail::count_of<T, Types...>() == 1, "std::get_if<T>: T must occur exactly once");
  return std::get_if<ycxx::detail::index_of<T, Types...>()>(v);
}

} // namespace std

// =============================================================================================
// [variant.visit]
// =============================================================================================
namespace ycxx::detail {

template <class... Ts>
constexpr auto&& as_variant(std::variant<Ts...>& v) noexcept {
  return v;
}
template <class... Ts>
constexpr auto&& as_variant(const std::variant<Ts...>& v) noexcept {
  return v;
}
template <class... Ts>
constexpr auto&& as_variant(std::variant<Ts...>&& v) noexcept {
  return static_cast<std::variant<Ts...>&&>(v);
}
template <class... Ts>
constexpr auto&& as_variant(const std::variant<Ts...>&& v) noexcept {
  return static_cast<const std::variant<Ts...>&&>(v);
}
template <class V>
using as_variant_t = decltype(::ycxx::detail::as_variant(std::declval<V>()));

// GET<m>(v) on an already-validated index.
template <std::size_t I, class V>
constexpr decltype(auto) var_unchecked_get(V&& v) noexcept {
  using Alt = std::variant_alternative_t<I, std::remove_cvref_t<V>>;
  using Ref = copy_cvref<V&&, Alt>;
  return static_cast<Ref>(ycxx::detail::variant_access::raw<I>(v));
}

// Result type of the visitor for the all-zero index pack (the Mandates check compares every
// other combination against it).
template <class Vis, class... V>
using visit_result_t = decltype(::ycxx::detail::invoke(std::declval<Vis>(), ::ycxx::detail::var_unchecked_get<0>(std::declval<V>())...));

template <class R, bool Exact, class Vis>
constexpr R visit_finish(Vis&& vis, auto&&... alts) {
  // [variant.visit]/5 Mandates: one type and value category for every combination.
  static_assert(!Exact || std::is_same_v<decltype(::ycxx::detail::invoke(static_cast<Vis&&>(vis),
                                                                         static_cast<decltype(alts)&&>(alts)...)),
                                         R>,
                "std::visit: the visitor must return the same type and value category for all alternatives");
  if constexpr (std::is_void_v<R>)
    static_cast<void>(::ycxx::detail::invoke(static_cast<Vis&&>(vis), static_cast<decltype(alts)&&>(alts)...));
  else if constexpr (Exact) {
    return ::ycxx::detail::invoke(static_cast<Vis&&>(vis), static_cast<decltype(alts)&&>(alts)...);
  } else
    return ::ycxx::detail::invoke_r<R>(static_cast<Vis&&>(vis), static_cast<decltype(alts)&&>(alts)...);
}

// Bind alternatives left to right. `alts` are the already-selected alternatives.
template <class R, bool Exact, class Vis, class V, class... Vs>
constexpr R visit_bind(Vis&& vis, V&& v, Vs&&... vs) {
  constexpr std::size_t n = std::variant_size_v<std::remove_cvref_t<V>>;
  return ::ycxx::detail::dispatch_index<n>(v.index(), [&](auto i) -> R {
    decltype(auto) alt = ::ycxx::detail::var_unchecked_get<i>(static_cast<V&&>(v));
    if constexpr (sizeof...(Vs) == 0) {
      return ::ycxx::detail::visit_finish<R, Exact>(static_cast<Vis&&>(vis), static_cast<decltype(alt)&&>(alt));
    } else {
      // Curry: a visitor that receives the remaining alternatives and prepends `alt`.
      auto curried = [&](auto&&... rest) -> R {
        return ::ycxx::detail::visit_finish<R, Exact>(static_cast<Vis&&>(vis), static_cast<decltype(alt)&&>(alt),
                                      static_cast<decltype(rest)&&>(rest)...);
      };
      return ::ycxx::detail::visit_bind<R, true>(curried, static_cast<Vs&&>(vs)...);
    }
  });
}

template <bool Exact, class R, class Vis, class... V>
constexpr R visit_entry(Vis&& vis, V&&... vars) {
  if ((vars.valueless_by_exception() || ...))
    throw_bad_variant_access();
  if constexpr (sizeof...(V) == 0)
    return ::ycxx::detail::visit_finish<R, Exact>(static_cast<Vis&&>(vis));
  else
    return ::ycxx::detail::visit_bind<R, Exact>(static_cast<Vis&&>(vis), static_cast<V&&>(vars)...);
}

} // namespace ycxx::detail

namespace std {

template <class Visitor, class... Variants>
  requires(requires { typename ycxx::detail::as_variant_t<Variants>; } && ...)
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
  // The result type is computed in the body, not the signature: [variant.visit]/5 makes a
  // visitor that is not callable with every alternative a Mandates violation (a hard error),
  // not a reason to drop out of overload resolution.
  using R = ycxx::detail::visit_result_t<Visitor, ycxx::detail::as_variant_t<Variants>...>;
  return ycxx::detail::visit_entry<true, R>(static_cast<Visitor&&>(vis),
                                            ycxx::detail::as_variant(static_cast<Variants&&>(vars))...);
}
template <class R, class Visitor, class... Variants>
  requires(requires { typename ycxx::detail::as_variant_t<Variants>; } && ...)
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  return ycxx::detail::visit_entry<false, R>(static_cast<Visitor&&>(vis),
                                             ycxx::detail::as_variant(static_cast<Variants&&>(vars))...);
}

template <class... Types>
template <int, class Self, class Visitor>
constexpr decltype(auto) variant<Types...>::visit(this Self&& self, Visitor&& vis) {
  using V = ycxx::detail::copy_cvref<Self&&, variant>;
  // [variant.visit]/9 specifies (V)self: a C-style cast reaches an inaccessible (private) base.
  return std::visit(static_cast<Visitor&&>(vis), (V)self);
}
template <class... Types>
template <class R, class Self, class Visitor>
constexpr R variant<Types...>::visit(this Self&& self, Visitor&& vis) {
  using V = ycxx::detail::copy_cvref<Self&&, variant>;
  return std::visit<R>(static_cast<Visitor&&>(vis), (V)self);
}

// ---- [variant.relops] ----

template <class... Types>
  requires((requires(const Types& a) {
    { a == a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index())
    return false;
  if (v.valueless_by_exception())
    return true;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> bool {
    return static_cast<bool>(ycxx::detail::variant_access::raw<i>(v) == ycxx::detail::variant_access::raw<i>(w));
  });
}
template <class... Types>
  requires((requires(const Types& a) {
    { a != a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index())
    return true;
  if (v.valueless_by_exception())
    return false;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> bool {
    return static_cast<bool>(ycxx::detail::variant_access::raw<i>(v) != ycxx::detail::variant_access::raw<i>(w));
  });
}
template <class... Types>
  requires((requires(const Types& a) {
    { a < a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception())
    return false;
  if (v.valueless_by_exception())
    return true;
  if (v.index() < w.index())
    return true;
  if (v.index() > w.index())
    return false;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> bool {
    return static_cast<bool>(ycxx::detail::variant_access::raw<i>(v) < ycxx::detail::variant_access::raw<i>(w));
  });
}
template <class... Types>
  requires((requires(const Types& a) {
    { a > a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception())
    return false;
  if (w.valueless_by_exception())
    return true;
  if (v.index() > w.index())
    return true;
  if (v.index() < w.index())
    return false;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> bool {
    return static_cast<bool>(ycxx::detail::variant_access::raw<i>(v) > ycxx::detail::variant_access::raw<i>(w));
  });
}
template <class... Types>
  requires((requires(const Types& a) {
    { a <= a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception())
    return true;
  if (w.valueless_by_exception())
    return false;
  if (v.index() < w.index())
    return true;
  if (v.index() > w.index())
    return false;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> bool {
    return static_cast<bool>(ycxx::detail::variant_access::raw<i>(v) <= ycxx::detail::variant_access::raw<i>(w));
  });
}
template <class... Types>
  requires((requires(const Types& a) {
    { a >= a } -> convertible_to<bool>;
  }) && ...)
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception())
    return true;
  if (v.valueless_by_exception())
    return false;
  if (v.index() > w.index())
    return true;
  if (v.index() < w.index())
    return false;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> bool {
    return static_cast<bool>(ycxx::detail::variant_access::raw<i>(v) >= ycxx::detail::variant_access::raw<i>(w));
  });
}
template <class... Types>
  requires(three_way_comparable<Types> && ...)
constexpr common_comparison_category_t<compare_three_way_result_t<Types>...> operator<=>(const variant<Types...>& v,
                                                                                        const variant<Types...>& w) {
  using R = common_comparison_category_t<compare_three_way_result_t<Types>...>;
  if (v.valueless_by_exception() && w.valueless_by_exception())
    return strong_ordering::equal;
  if (v.valueless_by_exception())
    return strong_ordering::less;
  if (w.valueless_by_exception())
    return strong_ordering::greater;
  if (auto c = v.index() <=> w.index(); c != 0)
    return c;
  return ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> R {
    return ycxx::detail::variant_access::raw<i>(v) <=> ycxx::detail::variant_access::raw<i>(w);
  });
}

// ---- [variant.specalg] ----
template <class... Types>
  requires((is_move_constructible_v<Types> && is_swappable_v<Types>) && ...)
constexpr void swap(variant<Types...>& v, variant<Types...>& w) noexcept(noexcept(v.swap(w))) {
  v.swap(w);
}

// ---- [variant.hash] ----
template <class... Types>
  requires(ycxx::detail::hash_enabled<remove_const_t<Types>> && ...)
struct hash<variant<Types...>> {
  size_t operator()(const variant<Types...>& v) const {
    if (v.valueless_by_exception())
      return static_cast<size_t>(0x76616c75u);
    size_t h = ycxx::detail::dispatch_index<sizeof...(Types)>(v.index(), [&](auto i) -> size_t {
      return hash<remove_const_t<Types...[i]>>{}(ycxx::detail::variant_access::raw<i>(v));
    });
    return static_cast<size_t>(ycxx::detail::mum(h ^ ycxx::detail::hash_k1, v.index() ^ ycxx::detail::hash_k2));
  }
};

} // namespace std
