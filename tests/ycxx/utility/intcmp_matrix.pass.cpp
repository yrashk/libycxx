// [utility.intcmp]: for every pair of standard integer types T, U ([basic.fundamental]/1-2:
// signed char, short, int, long, long long and their unsigned counterparts):
//   cmp_equal(t, u): "if constexpr (is_signed_v<T> == is_signed_v<U>) return t == u; else if
//   constexpr (is_signed_v<T>) return t < 0 ? false : make_unsigned_t<T>(t) == u; else ..."
//   i.e. comparison of the mathematical values; cmp_not_equal = !cmp_equal, cmp_less,
//   cmp_greater(t, u) = cmp_less(u, t), cmp_less_equal = !cmp_less(u, t),
//   cmp_greater_equal = !cmp_less(t, u). All are constexpr and noexcept.
//   in_range<R>(t): "Returns: true if the value of t is in the range of values that can be
//   represented in R, and false otherwise."
// Checked exhaustively over min, min+1, -1, 0, 1, max-1, max of each type (as values of each
// type), in constant evaluation and at run time, against comparisons done in __int128 (or, when
// unavailable, a sign/magnitude reference).
#include <utility>
#include <limits>
#include <type_traits>
#include "check.hpp"

struct Big {   // exact mathematical value of any 64-bit-or-narrower integer: sign + magnitude
  bool neg;
  unsigned long long mag;
  template <class T> static constexpr Big of(T v) {
    if constexpr (std::is_signed_v<T>) {
      if (v < 0) return {true, 0ull - static_cast<unsigned long long>(static_cast<long long>(v))};
    }
    return {false, static_cast<unsigned long long>(v)};
  }
  friend constexpr bool operator==(Big a, Big b) { return (a.mag == 0 && b.mag == 0) || (a.neg == b.neg && a.mag == b.mag); }
  friend constexpr bool operator<(Big a, Big b) {
    if (a == b) return false;
    if (a.neg != b.neg) return a.neg;
    return a.neg ? a.mag > b.mag : a.mag < b.mag;
  }
};

template <class T> constexpr int nvals = 7;
template <class T> constexpr T val(int i) {
  using L = std::numeric_limits<T>;
  switch (i) {
    case 0: return L::min();
    case 1: return static_cast<T>(L::min() + 1);
    case 2: return static_cast<T>(-1);   // the maximum for an unsigned type
    case 3: return 0;
    case 4: return 1;
    case 5: return static_cast<T>(L::max() - 1);
    default: return L::max();
  }
}

template <class T, class U> constexpr bool pair_ok() {
  static_assert(noexcept(std::cmp_equal(T(), U())) && noexcept(std::cmp_not_equal(T(), U())) &&
                noexcept(std::cmp_less(T(), U())) && noexcept(std::cmp_greater(T(), U())) &&
                noexcept(std::cmp_less_equal(T(), U())) && noexcept(std::cmp_greater_equal(T(), U())) &&
                noexcept(std::in_range<T>(U())));
  static_assert(std::is_same_v<decltype(std::cmp_less(T(), U())), bool> &&
                std::is_same_v<decltype(std::in_range<T>(U())), bool>);
  for (int i = 0; i < nvals<T>; ++i) {
    for (int j = 0; j < nvals<U>; ++j) {
      const T t = val<T>(i);
      const U u = val<U>(j);
      const Big bt = Big::of(t), bu = Big::of(u);
      if (std::cmp_equal(t, u) != (bt == bu)) return false;
      if (std::cmp_not_equal(t, u) != !(bt == bu)) return false;
      if (std::cmp_less(t, u) != (bt < bu)) return false;
      if (std::cmp_greater(t, u) != (bu < bt)) return false;
      if (std::cmp_less_equal(t, u) != !(bu < bt)) return false;
      if (std::cmp_greater_equal(t, u) != !(bt < bu)) return false;
    }
    // in_range<U>(t): t lies within [min(U), max(U)].
    const Big bt = Big::of(val<T>(i));
    const bool expect = !(bt < Big::of(std::numeric_limits<U>::min())) && !(Big::of(std::numeric_limits<U>::max()) < bt);
    if (std::in_range<U>(val<T>(i)) != expect) return false;
  }
  return true;
}

template <class T, class... Us> constexpr bool row() { return (pair_ok<T, Us>() && ...); }
template <class... Ts> constexpr bool all() { return (row<Ts, Ts...>() && ...); }

#define TYPES signed char, short, int, long, long long, unsigned char, unsigned short, unsigned, \
              unsigned long, unsigned long long
static_assert(all<TYPES>());
// cv-qualified arguments deduce the cv-unqualified type; const objects still compare.
static_assert(std::cmp_less(static_cast<const int>(-1), static_cast<const unsigned>(0)));

int main() {
  // Run time (non-constant) evaluation.
  volatile bool sink = true;
  bool ok = all<TYPES>();
  sink = ok;
  CHECK(sink);
  volatile int m1 = -1;
  volatile unsigned long long big = std::numeric_limits<unsigned long long>::max();
  CHECK(std::cmp_less(m1, big));
  CHECK(!std::cmp_equal(m1, big));
  CHECK(!std::in_range<unsigned char>(m1));
  CHECK(std::in_range<signed char>(m1));
  CHECK(!std::in_range<long long>(big));
}
