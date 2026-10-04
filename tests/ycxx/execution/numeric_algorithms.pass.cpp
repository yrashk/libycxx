// Parallel overloads of the <numeric> algorithms, with each standard policy.
// [algorithms.parallel.overloads]/2: same semantics as the overload without the policy.
// [reduce]: reduce(exec, first, last) = reduce(exec, first, last, value_type{}), the result is
// GENERALIZED_SUM(binary_op, init, *i...) (so exact for an associative, commutative op);
// [transform.reduce]: the three forms; [inclusive.scan], [exclusive.scan],
// [transform.inclusive.scan], [transform.exclusive.scan]: the K-th output is the generalized
// noncommutative sum of the first K + 1 (inclusive, init folded in first when given) or the
// first K (exclusive, starting from init) inputs; "Remarks: result may be equal to first"
// ([inclusive.scan]/9, [exclusive.scan]/8); [adjacent.difference]: *result = *first, then
// *(result + k) = op(*(first + k), *(first + k - 1)); for the overloads with an ExecutionPolicy
// the input and output ranges do not overlap (/3 Remarks). Return values: the end of the
// output range, or the sum.
#include <numeric>
#include <execution>
#include <functional>
#include <type_traits>
#include "check.hpp"

constexpr int N = 2000;

template <class P>
void run(const P& pol) {
  static long long a[N], b[N], c[N];
  for (int i = 0; i < N; ++i) a[i] = i + 1;
  const long long S = 1LL * N * (N + 1) / 2;

  static_assert(std::is_same_v<decltype(std::reduce(pol, a, a + N)), long long>);
  CHECK(std::reduce(pol, a, a + N) == S);
  CHECK(std::reduce(pol, a, a + N, 10LL) == S + 10);
  static_assert(std::is_same_v<decltype(std::reduce(pol, a, a + N, 0.0)), double>);
  CHECK(std::reduce(pol, a, a + N, 0LL, [](long long x, long long y) { return x > y ? x : y; }) == N);
  CHECK(std::reduce(pol, a, a, 7LL) == 7);  // empty range: init

  long long sq = 0;
  for (int i = 1; i <= N; ++i) sq += 1LL * i * i;
  CHECK(std::transform_reduce(pol, a, a + N, a, 0LL) == sq);
  CHECK(std::transform_reduce(pol, a, a + N, a, 1LL, std::plus<>(), std::multiplies<>()) == sq + 1);
  CHECK(std::transform_reduce(pol, a, a + N, 0LL, std::plus<>(), [](long long x) { return 2 * x; }) == 2 * S);

  CHECK(std::inclusive_scan(pol, a, a + N, b) == b + N);
  for (int i = 0; i < N; ++i) CHECK(b[i] == 1LL * (i + 1) * (i + 2) / 2);
  CHECK(std::inclusive_scan(pol, a, a + N, b, std::plus<>()) == b + N);
  CHECK(b[N - 1] == S);
  CHECK(std::inclusive_scan(pol, a, a + N, b, std::plus<>(), 100LL) == b + N);
  CHECK(b[0] == 101 && b[N - 1] == S + 100);

  CHECK(std::exclusive_scan(pol, a, a + N, b, 0LL) == b + N);
  for (int i = 0; i < N; ++i) CHECK(b[i] == 1LL * i * (i + 1) / 2);
  CHECK(std::exclusive_scan(pol, a, a + N, b, 5LL, std::plus<>()) == b + N);
  CHECK(b[0] == 5 && b[N - 1] == 5 + S - N);

  CHECK(std::transform_inclusive_scan(pol, a, a + N, b, std::plus<>(), [](long long x) { return -x; }) == b + N);
  CHECK(b[N - 1] == -S);
  CHECK(std::transform_inclusive_scan(pol, a, a + N, b, std::plus<>(), [](long long x) { return 2 * x; }, 1LL) == b + N);
  CHECK(b[0] == 3 && b[N - 1] == 2 * S + 1);
  CHECK(std::transform_exclusive_scan(pol, a, a + N, b, 0LL, std::plus<>(), [](long long x) { return 2 * x; }) == b + N);
  CHECK(b[0] == 0 && b[N - 1] == 2 * (S - N));

  // in place (result == first)
  for (int i = 0; i < N; ++i) c[i] = 1;
  CHECK(std::inclusive_scan(pol, c, c + N, c) == c + N);
  for (int i = 0; i < N; ++i) CHECK(c[i] == i + 1);
  CHECK(std::exclusive_scan(pol, c, c + N, c, 0LL) == c + N);
  for (int i = 0; i < N; ++i) CHECK(c[i] == 1LL * i * (i + 1) / 2);

  // a non-commutative but associative operation: the order of the operands is kept
  // (GENERALIZED_NONCOMMUTATIVE_SUM): 2x2 matrix-like pairs combined as affine maps
  struct Aff {
    long long m, k;  // x -> m x + k
  };
  static Aff f[64], g[64];
  for (int i = 0; i < 64; ++i) f[i] = Aff{(i % 3) + 1, i};
  auto compose = [](Aff p, Aff q) { return Aff{p.m * q.m % 1000003, (q.m * p.k + q.k) % 1000003}; };  // q after p
  std::inclusive_scan(pol, f, f + 64, g, compose);
  Aff acc = f[0];
  CHECK(g[0].m == acc.m && g[0].k == acc.k);
  for (int i = 1; i < 64; ++i) {
    acc = compose(acc, f[i]);
    CHECK(g[i].m == acc.m && g[i].k == acc.k);
  }
  std::exclusive_scan(pol, f, f + 64, g, Aff{1, 0}, compose);
  acc = Aff{1, 0};
  for (int i = 0; i < 64; ++i) {
    CHECK(g[i].m == acc.m && g[i].k == acc.k);
    acc = compose(acc, f[i]);
  }

  CHECK(std::adjacent_difference(pol, a, a + N, b) == b + N);
  CHECK(b[0] == 1);
  for (int i = 1; i < N; ++i) CHECK(b[i] == 1);
  CHECK(std::adjacent_difference(pol, a, a + N, b, [](long long cur, long long prev) { return cur * 10 + prev; }) == b + N);
  CHECK(b[0] == 1 && b[1] == 21 && b[N - 1] == 10LL * N + (N - 1));
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
