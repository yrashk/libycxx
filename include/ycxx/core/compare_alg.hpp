// libycxx core: comparison algorithms ([cmp.alg]) and type ordering ([compare.type]).
#pragma once

#include <ycxx/core/compare.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// ---- IEEE totalOrder key for floating-point values -------------------------------------------
// Maps a floating-point value to (sign, magnitude-key) such that comparing keys implements
// IEEE 754 totalOrder (-NaN < -inf < ... < -0 < +0 < ... < +inf < +NaN).
struct fp_key {
  bool negative;
  uint128 magnitude; // exponent and significand bits, as an unsigned integer
};

struct x87_layout {
  unsigned long long significand;
  unsigned short sign_exponent;
};

template <class T>
constexpr fp_key fp_total_key(T x) noexcept {
  constexpr fp_format_info f = fp_format<T>;
  if constexpr (__is_same(T, long double) && f.digits == 64) {
    auto r = __builtin_bit_cast(x87_layout, x);
    return {static_cast<bool>(r.sign_exponent >> 15),
            (static_cast<uint128>(r.sign_exponent & 0x7fff) << 64) | r.significand};
  } else {
    using U = std::conditional_t<sizeof(T) == 2, unsigned short,
                                 std::conditional_t<sizeof(T) == 4, unsigned int,
                                                    std::conditional_t<sizeof(T) == 8, unsigned long long, uint128>>>;
    constexpr int bits = static_cast<int>(sizeof(T) * __CHAR_BIT__);
    U u = __builtin_bit_cast(U, x);
    U sign = U(U(1) << (bits - 1));
    return {(u & sign) != 0, static_cast<uint128>(u & U(~sign))};
  }
}

template <class T>
constexpr std::strong_ordering fp_strong_order(T a, T b) noexcept {
  fp_key ka = fp_total_key(a), kb = fp_total_key(b);
  if (ka.negative != kb.negative)
    return ka.negative ? std::strong_ordering::less : std::strong_ordering::greater;
  if (ka.magnitude == kb.magnitude)
    return std::strong_ordering::equal;
  bool less = ka.magnitude < kb.magnitude;
  return (less != ka.negative) ? std::strong_ordering::less : std::strong_ordering::greater;
}

template <class T>
constexpr std::weak_ordering fp_weak_order(T a, T b) noexcept {
  bool na = a != a, nb = b != b;
  if (!na && !nb) { // ordinary comparison; -0 == +0
    if (a < b)
      return std::weak_ordering::less;
    if (b < a)
      return std::weak_ordering::greater;
    return std::weak_ordering::equivalent;
  }
  // NaNs: all negative NaNs are least, all positive NaNs greatest.
  auto rank = [](T v, bool nan) { return nan ? (fp_total_key(v).negative ? -1 : 1) : 0; };
  int ra = rank(a, na), rb = rank(b, nb);
  if (ra == rb)
    return std::weak_ordering::equivalent;
  return ra < rb ? std::weak_ordering::less : std::weak_ordering::greater;
}

// ---- CPOs ----------------------------------------------------------------------------------
// Each call is expression-equivalent to the expression [cmp.alg] selects, so it is noexcept
// exactly when that expression is. `choose` names the selected branch (none: the call is
// ill-formed) and that expression's exception specification; the call operators are constrained
// on the first and carry the second.
namespace cmp_cpo {

void strong_order() = delete;
void weak_order() = delete;
void partial_order() = delete;

template <class E, class F>
concept same_decayed = __is_same(std::decay_t<E>, std::decay_t<F>);

template <class E, class F>
concept adl_strong = requires(E&& e, F&& f) { std::strong_ordering(strong_order(static_cast<E&&>(e), static_cast<F&&>(f))); };
template <class E, class F>
concept adl_weak = requires(E&& e, F&& f) { std::weak_ordering(weak_order(static_cast<E&&>(e), static_cast<F&&>(f))); };
template <class E, class F>
concept adl_partial = requires(E&& e, F&& f) { std::partial_ordering(partial_order(static_cast<E&&>(e), static_cast<F&&>(f))); };

template <class Cat, class E, class F>
concept three_way_as = requires(E&& e, F&& f) { Cat(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f))); };

template <class E>
concept floating = is_floating_v<std::decay_t<E>>;

enum class cmp_branch : unsigned char { none, adl, floating, three_way, stronger, operators };
struct cmp_choice {
  cmp_branch branch;
  bool nothrow;
};

struct strong_order_fn {
  template <class E, class F>
  static consteval cmp_choice choose() {
    if constexpr (!same_decayed<E, F>)
      return {cmp_branch::none, false};
    else if constexpr (adl_strong<E, F>)
      return {cmp_branch::adl, noexcept(std::strong_ordering(strong_order(std::declval<E>(), std::declval<F>())))};
    else if constexpr (floating<E>)
      return {cmp_branch::floating, true};
    else if constexpr (three_way_as<std::strong_ordering, E, F>)
      return {cmp_branch::three_way,
              noexcept(std::strong_ordering(std::compare_three_way()(std::declval<E>(), std::declval<F>())))};
    else
      return {cmp_branch::none, false};
  }

  template <class E, class F>
    requires(choose<E, F>().branch != cmp_branch::none)
  constexpr std::strong_ordering operator()(E&& e, F&& f) const noexcept(choose<E, F>().nothrow) {
    constexpr cmp_branch b = choose<E, F>().branch;
    if constexpr (b == cmp_branch::adl)
      return std::strong_ordering(strong_order(static_cast<E&&>(e), static_cast<F&&>(f)));
    else if constexpr (b == cmp_branch::floating)
      return fp_strong_order<std::decay_t<E>>(e, f);
    else
      return std::strong_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
  }
};
inline constexpr strong_order_fn strong_order_obj{};

struct weak_order_fn {
  template <class E, class F>
  static consteval cmp_choice choose() {
    if constexpr (!same_decayed<E, F>)
      return {cmp_branch::none, false};
    else if constexpr (adl_weak<E, F>)
      return {cmp_branch::adl, noexcept(std::weak_ordering(weak_order(std::declval<E>(), std::declval<F>())))};
    else if constexpr (floating<E>)
      return {cmp_branch::floating, true};
    else if constexpr (three_way_as<std::weak_ordering, E, F>)
      return {cmp_branch::three_way,
              noexcept(std::weak_ordering(std::compare_three_way()(std::declval<E>(), std::declval<F>())))};
    else if constexpr (requires { std::weak_ordering(strong_order_obj(std::declval<E>(), std::declval<F>())); })
      return {cmp_branch::stronger, noexcept(std::weak_ordering(strong_order_obj(std::declval<E>(), std::declval<F>())))};
    else
      return {cmp_branch::none, false};
  }

  template <class E, class F>
    requires(choose<E, F>().branch != cmp_branch::none)
  constexpr std::weak_ordering operator()(E&& e, F&& f) const noexcept(choose<E, F>().nothrow) {
    constexpr cmp_branch b = choose<E, F>().branch;
    if constexpr (b == cmp_branch::adl)
      return std::weak_ordering(weak_order(static_cast<E&&>(e), static_cast<F&&>(f)));
    else if constexpr (b == cmp_branch::floating)
      return fp_weak_order<std::decay_t<E>>(e, f);
    else if constexpr (b == cmp_branch::three_way)
      return std::weak_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
    else
      return std::weak_ordering(strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)));
  }
};
inline constexpr weak_order_fn weak_order_obj{};

struct partial_order_fn {
  template <class E, class F>
  static consteval cmp_choice choose() {
    if constexpr (!same_decayed<E, F>)
      return {cmp_branch::none, false};
    else if constexpr (adl_partial<E, F>)
      return {cmp_branch::adl, noexcept(std::partial_ordering(partial_order(std::declval<E>(), std::declval<F>())))};
    else if constexpr (three_way_as<std::partial_ordering, E, F>)
      return {cmp_branch::three_way,
              noexcept(std::partial_ordering(std::compare_three_way()(std::declval<E>(), std::declval<F>())))};
    else if constexpr (requires { std::partial_ordering(weak_order_obj(std::declval<E>(), std::declval<F>())); })
      return {cmp_branch::stronger, noexcept(std::partial_ordering(weak_order_obj(std::declval<E>(), std::declval<F>())))};
    else
      return {cmp_branch::none, false};
  }

  template <class E, class F>
    requires(choose<E, F>().branch != cmp_branch::none)
  constexpr std::partial_ordering operator()(E&& e, F&& f) const noexcept(choose<E, F>().nothrow) {
    constexpr cmp_branch b = choose<E, F>().branch;
    if constexpr (b == cmp_branch::adl)
      return std::partial_ordering(partial_order(static_cast<E&&>(e), static_cast<F&&>(f)));
    else if constexpr (b == cmp_branch::three_way)
      return std::partial_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
    else
      return std::partial_ordering(weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)));
  }
};
inline constexpr partial_order_fn partial_order_obj{};

// The fallbacks' operator branches evaluate E and F once each, as the lvalues e and f.
template <class E, class F>
concept eq_lt_testable = requires(E&& e, F&& f) {
  { e == f } -> boolean_testable;
  { e < f } -> boolean_testable;
};

struct strong_fallback_fn {
  template <class E, class F>
  static consteval cmp_choice choose() {
    if constexpr (!same_decayed<E, F>)
      return {cmp_branch::none, false};
    else if constexpr (requires { strong_order_obj(std::declval<E>(), std::declval<F>()); })
      return {cmp_branch::stronger, noexcept(strong_order_obj(std::declval<E>(), std::declval<F>()))};
    else if constexpr (eq_lt_testable<E, F>)
      return {cmp_branch::operators,
              noexcept(std::declval<E&>() == std::declval<F&>()  ? std::strong_ordering::equal
                       : std::declval<E&>() < std::declval<F&>() ? std::strong_ordering::less
                                                                 : std::strong_ordering::greater)};
    else
      return {cmp_branch::none, false};
  }

  template <class E, class F>
    requires(choose<E, F>().branch != cmp_branch::none)
  constexpr std::strong_ordering operator()(E&& e, F&& f) const noexcept(choose<E, F>().nothrow) {
    if constexpr (choose<E, F>().branch == cmp_branch::stronger)
      return strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f));
    else
      return e == f ? std::strong_ordering::equal : e < f ? std::strong_ordering::less : std::strong_ordering::greater;
  }
};
struct weak_fallback_fn {
  template <class E, class F>
  static consteval cmp_choice choose() {
    if constexpr (!same_decayed<E, F>)
      return {cmp_branch::none, false};
    else if constexpr (requires { weak_order_obj(std::declval<E>(), std::declval<F>()); })
      return {cmp_branch::stronger, noexcept(weak_order_obj(std::declval<E>(), std::declval<F>()))};
    else if constexpr (eq_lt_testable<E, F>)
      return {cmp_branch::operators,
              noexcept(std::declval<E&>() == std::declval<F&>()  ? std::weak_ordering::equivalent
                       : std::declval<E&>() < std::declval<F&>() ? std::weak_ordering::less
                                                                 : std::weak_ordering::greater)};
    else
      return {cmp_branch::none, false};
  }

  template <class E, class F>
    requires(choose<E, F>().branch != cmp_branch::none)
  constexpr std::weak_ordering operator()(E&& e, F&& f) const noexcept(choose<E, F>().nothrow) {
    if constexpr (choose<E, F>().branch == cmp_branch::stronger)
      return weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f));
    else
      return e == f ? std::weak_ordering::equivalent : e < f ? std::weak_ordering::less : std::weak_ordering::greater;
  }
};
struct partial_fallback_fn {
  template <class E, class F>
  static consteval cmp_choice choose() {
    if constexpr (!same_decayed<E, F>)
      return {cmp_branch::none, false};
    else if constexpr (requires { partial_order_obj(std::declval<E>(), std::declval<F>()); })
      return {cmp_branch::stronger, noexcept(partial_order_obj(std::declval<E>(), std::declval<F>()))};
    else if constexpr (requires(E&& e, F&& f) {
                         { e == f } -> boolean_testable;
                         { e < f } -> boolean_testable;
                         { f < e } -> boolean_testable;
                       })
      return {cmp_branch::operators,
              noexcept(std::declval<E&>() == std::declval<F&>()  ? std::partial_ordering::equivalent
                       : std::declval<E&>() < std::declval<F&>() ? std::partial_ordering::less
                       : std::declval<F&>() < std::declval<E&>() ? std::partial_ordering::greater
                                                                 : std::partial_ordering::unordered)};
    else
      return {cmp_branch::none, false};
  }

  template <class E, class F>
    requires(choose<E, F>().branch != cmp_branch::none)
  constexpr std::partial_ordering operator()(E&& e, F&& f) const noexcept(choose<E, F>().nothrow) {
    if constexpr (choose<E, F>().branch == cmp_branch::stronger)
      return partial_order_obj(static_cast<E&&>(e), static_cast<F&&>(f));
    else
      return e == f  ? std::partial_ordering::equivalent
             : e < f ? std::partial_ordering::less
             : f < e ? std::partial_ordering::greater
                     : std::partial_ordering::unordered;
  }
};

} // namespace cmp_cpo

// ---- type ordering --------------------------------------------------------------------------
// The implementation-defined total order on types: lexicographic order of the compiler's
// spelling of a function signature that mentions the type. Same type => same spelling;
// distinct types visible in one TU => distinct spellings.
template <class T>
consteval const char* type_spelling() {
  return __PRETTY_FUNCTION__;
}
template <class T, class U>
consteval int type_compare() {
  if constexpr (__is_same(T, U)) {
    return 0;
  } else {
    const char* a = type_spelling<T>();
    const char* b = type_spelling<U>();
    for (;; ++a, ++b) {
      if (*a != *b)
        return static_cast<unsigned char>(*a) < static_cast<unsigned char>(*b) ? -1 : 1;
      if (*a == '\0')
        return 0; // unreachable for distinct types
    }
  }
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

inline namespace cpo {
inline constexpr ycxx::detail::cmp_cpo::strong_order_fn strong_order{};
inline constexpr ycxx::detail::cmp_cpo::weak_order_fn weak_order{};
inline constexpr ycxx::detail::cmp_cpo::partial_order_fn partial_order{};
inline constexpr ycxx::detail::cmp_cpo::strong_fallback_fn compare_strong_order_fallback{};
inline constexpr ycxx::detail::cmp_cpo::weak_fallback_fn compare_weak_order_fallback{};
inline constexpr ycxx::detail::cmp_cpo::partial_fallback_fn compare_partial_order_fallback{};
} // namespace cpo

template <class T, class U>
struct type_order {
  static constexpr strong_ordering value = ycxx::detail::type_compare<T, U>() < 0    ? strong_ordering::less
                                           : ycxx::detail::type_compare<T, U>() == 0 ? strong_ordering::equal
                                                                                     : strong_ordering::greater;
  using value_type = strong_ordering;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};
template <class T, class U>
constexpr strong_ordering type_order_v = type_order<T, U>::value;

} // namespace std
