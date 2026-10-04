// libycxx core: comparison algorithms ([cmp.alg]) and type ordering ([compare.type]).
#pragma once

#include <ycxx/core/compare.hpp>

namespace ycxx::detail {

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

template <class E, class F>
concept three_way_as = requires(E&& e, F&& f) { std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)); };

template <class E>
concept floating = is_floating_v<std::decay_t<E>>;

struct strong_order_fn {
  template <class E, class F>
    requires same_decayed<E, F> && (adl_strong<E, F> || floating<E> ||
                                    requires(E&& e, F&& f) {
                                      std::strong_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
                                    })
  constexpr std::strong_ordering operator()(E&& e, F&& f) const {
    if constexpr (adl_strong<E, F>)
      return std::strong_ordering(strong_order(static_cast<E&&>(e), static_cast<F&&>(f)));
    else if constexpr (floating<E>)
      return fp_strong_order<std::decay_t<E>>(e, f);
    else
      return std::strong_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
  }
};
inline constexpr strong_order_fn strong_order_obj{};

struct weak_order_fn {
  template <class E, class F>
    requires same_decayed<E, F> &&
             (adl_weak<E, F> || floating<E> ||
              requires(E&& e, F&& f) {
                std::weak_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
              } || requires(E&& e, F&& f) { std::weak_ordering(strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f))); })
  constexpr std::weak_ordering operator()(E&& e, F&& f) const {
    if constexpr (adl_weak<E, F>)
      return std::weak_ordering(weak_order(static_cast<E&&>(e), static_cast<F&&>(f)));
    else if constexpr (floating<E>)
      return fp_weak_order<std::decay_t<E>>(e, f);
    else if constexpr (requires {
                         std::weak_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
                       })
      return std::weak_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
    else
      return std::weak_ordering(strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)));
  }
};
inline constexpr weak_order_fn weak_order_obj{};

struct partial_order_fn {
  template <class E, class F>
    requires same_decayed<E, F> &&
             (adl_partial<E, F> ||
              requires(E&& e, F&& f) {
                std::partial_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
              } || requires(E&& e, F&& f) { std::partial_ordering(weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f))); })
  constexpr std::partial_ordering operator()(E&& e, F&& f) const {
    if constexpr (adl_partial<E, F>)
      return std::partial_ordering(partial_order(static_cast<E&&>(e), static_cast<F&&>(f)));
    else if constexpr (requires {
                         std::partial_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
                       })
      return std::partial_ordering(std::compare_three_way()(static_cast<E&&>(e), static_cast<F&&>(f)));
    else
      return std::partial_ordering(weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)));
  }
};
inline constexpr partial_order_fn partial_order_obj{};

template <class E, class F>
concept eq_lt_testable = requires(E&& e, F&& f) {
  { e == f } -> boolean_testable;
  { e < f } -> boolean_testable;
};

struct strong_fallback_fn {
  template <class E, class F>
    requires same_decayed<E, F> &&
             (requires(E&& e, F&& f) { strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)); } ||
              eq_lt_testable<E, F>)
  constexpr std::strong_ordering operator()(E&& e, F&& f) const {
    if constexpr (requires { strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)); })
      return strong_order_obj(static_cast<E&&>(e), static_cast<F&&>(f));
    else
      return e == f ? std::strong_ordering::equal : e < f ? std::strong_ordering::less : std::strong_ordering::greater;
  }
};
struct weak_fallback_fn {
  template <class E, class F>
    requires same_decayed<E, F> &&
             (requires(E&& e, F&& f) { weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)); } ||
              eq_lt_testable<E, F>)
  constexpr std::weak_ordering operator()(E&& e, F&& f) const {
    if constexpr (requires { weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)); })
      return weak_order_obj(static_cast<E&&>(e), static_cast<F&&>(f));
    else
      return e == f ? std::weak_ordering::equivalent : e < f ? std::weak_ordering::less : std::weak_ordering::greater;
  }
};
struct partial_fallback_fn {
  template <class E, class F>
    requires same_decayed<E, F> &&
             (requires(E&& e, F&& f) { partial_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)); } ||
              requires(E&& e, F&& f) {
                { e == f } -> boolean_testable;
                { e < f } -> boolean_testable;
                { f < e } -> boolean_testable;
              })
  constexpr std::partial_ordering operator()(E&& e, F&& f) const {
    if constexpr (requires { partial_order_obj(static_cast<E&&>(e), static_cast<F&&>(f)); })
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

} // namespace ycxx::detail

namespace std {

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
