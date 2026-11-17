// FLAGS: -O2
// [simd.binary], [simd.unary], [simd.cassign], [simd.reductions]: the element-wise operators of a
// basic_vec of signed char or short give the scalar operator's result, which is computed in int
// and converted back to the element type ([conv.integral]/3: modulo 2^N): 127 + 1 is -128,
// -(-128) is -128, 100 * 100 is 16 for signed char, (-128) / -1 is -128 and (-128) % -1 is 0. A
// wrapping element is not an overflow, so an optimizer may not assume that v + 1 > v (built with
// -O2, which GCC miscompiled when the operation was done in the narrow signed vector type).
// reduce with plus<> and multiplies<> wraps the same way. Checked at run time on values the
// compiler cannot see, and in constant evaluation, where an overflow is not a constant
// expression. Power-of-two widths (vector chunks), one chunk and several.
#include <functional>
#include <limits>
#include <simd>
#include "check.hpp"

namespace simd = std::simd;

template <class T>
constexpr T wrap(int x) {
  return static_cast<T>(x);
}

// Every operator on V(x) and V(y), against the scalar operators; true if all agree.
template <class V>
constexpr bool agrees(typename V::value_type x, typename V::value_type y) {
  using T = typename V::value_type;
  const V a(x), b(y);
  V inc = a, dec = a, post_inc = a, post_dec = a, cadd = a, csub = a, cmul = a;
  ++inc;
  --dec;
  post_inc++;
  post_dec--;
  cadd += b;
  csub -= b;
  cmul *= b;
  bool ok = simd::all_of(a + b == V(wrap<T>(x + y))) && simd::all_of(a - b == V(wrap<T>(x - y))) &&
            simd::all_of(a * b == V(wrap<T>(x * y))) && simd::all_of(-a == V(wrap<T>(-x))) &&
            simd::all_of(inc == V(wrap<T>(x + 1))) && simd::all_of(dec == V(wrap<T>(x - 1))) &&
            simd::all_of(post_inc == inc) && simd::all_of(post_dec == dec) && simd::all_of(cadd == a + b) &&
            simd::all_of(csub == a - b) && simd::all_of(cmul == a * b);
  if (y != 0)
    ok = ok && simd::all_of(a / b == V(wrap<T>(x / y))) && simd::all_of(a % b == V(wrap<T>(x % y)));
  ok = ok && simd::reduce(a) == wrap<T>(x * V::size()) && simd::reduce(a, std::plus<>()) == wrap<T>(x * V::size());
  int prod = 1;
  for (int i = 0; i < V::size(); ++i)
    prod = static_cast<int>(static_cast<unsigned>(prod) * static_cast<unsigned>(x));
  return ok && simd::reduce(a, std::multiplies<>()) == wrap<T>(prod);
}

// The comparisons an optimizer that assumes no overflow folds to true.
template <class V>
[[gnu::noinline]] bool grows(V v) {
  return simd::all_of(v + V(typename V::value_type(1)) > v);
}
template <class V>
[[gnu::noinline]] bool shrinks(V v) {
  return simd::all_of(v - V(typename V::value_type(1)) < v);
}
template <class V>
[[gnu::noinline]] bool grows_inc(V v) {
  V w = v;
  ++w;
  return simd::all_of(w > v);
}
template <class V>
[[gnu::noinline]] bool shrinks_dec(V v) {
  V w = v;
  --w;
  return simd::all_of(w < v);
}
template <class V>
[[gnu::noinline]] bool doubles(V v) {
  return simd::all_of(v * V(typename V::value_type(2)) > v);
}
template <class V>
[[gnu::noinline]] bool negates(V v) {
  return simd::all_of(-v > V(typename V::value_type(0)));
}
template <class V>
[[gnu::noinline]] bool sums_up(V v) {
  return simd::reduce(v) > v[0];
}

template <class V>
void run() {
  using T = typename V::value_type;
  using L = std::numeric_limits<T>;
  static_assert(agrees<V>(L::max(), 1) && agrees<V>(L::min(), 1) && agrees<V>(L::min(), -1) &&
                agrees<V>(L::max(), L::max()) && agrees<V>(L::min(), L::min()) && agrees<V>(100, 3) &&
                agrees<V>(-100, 7) && agrees<V>(3, -5));

  volatile T vmax = L::max(), vmin = L::min(), vbig = L::max() / 2 + 1, vone = 1, vneg = -1, vhundred = 100;
  const T mx = vmax, mn = vmin, big = vbig, one = vone, neg = vneg, hundred = vhundred;
  CHECK(!grows(V(mx)) && grows(V(one)));
  CHECK(!shrinks(V(mn)) && shrinks(V(one)));
  CHECK(!grows_inc(V(mx)) && grows_inc(V(one)));
  CHECK(!shrinks_dec(V(mn)) && shrinks_dec(V(one)));
  CHECK(!doubles(V(big)) && doubles(V(one)));
  CHECK(!negates(V(mn)) && negates(V(neg)));
  CHECK(!sums_up(V(mx)) && sums_up(V(one)));
  CHECK(agrees<V>(mx, one) && agrees<V>(mn, one) && agrees<V>(mn, neg) && agrees<V>(mx, mx) && agrees<V>(mn, mn) &&
        agrees<V>(hundred, 3) && agrees<V>(static_cast<T>(-hundred), 7) && agrees<V>(3, -5));

  // Lanes that differ: each element is its own scalar operation.
  const V a([&](int i) { return static_cast<T>(i % 2 == 0 ? mx - i : mn + i); });
  const V s = a + a, d = a - V(neg), p = a * a, n = -a;
  for (int i = 0; i < V::size(); ++i) {
    const int x = a[i];
    CHECK(s[i] == wrap<T>(x + x) && d[i] == wrap<T>(x + 1) && p[i] == wrap<T>(x * x) && n[i] == wrap<T>(-x));
  }
}

int main() {
  run<simd::vec<signed char, 16>>();
  run<simd::vec<signed char, 64>>();
  run<simd::vec<short, 8>>();
  run<simd::vec<short, 32>>();
  run<simd::vec<char, 16>>();
  return 0;
}
