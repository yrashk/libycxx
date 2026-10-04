// Parallel range algorithms (C++26, P3179): the overloads in namespace ranges taking an
// execution-policy Ep first, random_access_iterator + sized_sentinel_for or
// sized-random-access-range arguments ([algorithm.syn]). Semantics as the overloads without
// the policy ([algorithms.parallel.overloads]/2), with these differences spelled out in the
// draft:
// - [alg.foreach]: ranges::for_each(exec, ...) returns I / borrowed_iterator_t<R> (not an
//   in_fun_result); for_each_n(exec, first, n, f) returns first + n.
// - [alg.copy]/6-10: copy(exec, ...) takes an output range; N = min(last - first, result_last -
//   result) elements are copied; returns {first + N, result + N}.
// - [alg.transform]/1,4: N = min(M, result_last - result); unary returns {first1 + N, result +
//   N}, binary {first1 + N, first2 + N, result + N}.
// - [alg.fill]: fill(exec, r, value) returns the end iterator.
// - [sort], [alg.reverse], [alg.min.max], [alg.find], [alg.count], [alg.contains],
//   [alg.equal], [mismatch], [alg.remove], [alg.all.of]: results as without a policy.
// The return types are checked as declared in [algorithm.syn].
#include <algorithm>
#include <execution>
#include <functional>
#include <type_traits>
#include "check.hpp"

namespace r = std::ranges;
constexpr int N = 1000;

struct Rec {
  int key;
  int id;
};

template <class P>
void run(const P& pol) {
  static int a[N], b[N], c[N];
  for (int i = 0; i < N; ++i) a[i] = (i * 37) % N;

  CHECK(r::all_of(pol, a, [](int x) { return x < N; }));
  CHECK(r::any_of(pol, a, a + N, [](int x) { return x == 3; }));
  CHECK(r::none_of(pol, a, [](int x) { return x < 0; }));

  static_assert(std::is_same_v<decltype(r::for_each(pol, a, [](int) {})), int*>);
  static_assert(std::is_same_v<decltype(r::for_each(pol, a, a + N, [](int) {})), int*>);
  CHECK(r::for_each(pol, a, [](int& x) { ++x; }) == a + N);
  CHECK(r::for_each(pol, a, a + N, [](int& x) { --x; }) == a + N);
  CHECK(r::for_each_n(pol, a, 10, [](int& x) { x += 0; }) == a + 10);

  static_assert(std::is_same_v<decltype(r::find(pol, a, 74)), int*>);
  CHECK(r::find(pol, a, 74) == a + 2);
  CHECK(r::find(pol, a, a + N, -1) == a + N);
  CHECK(r::find_if(pol, a, [](int x) { return x > 100; }) == a + 3);
  CHECK(r::find(pol, r::subrange(a, a + N), 111) == a + 3);
  static_assert(std::is_same_v<decltype(r::count(pol, a, 1)), std::ptrdiff_t>);
  CHECK(r::count(pol, a, 37) == 1);
  CHECK(r::count_if(pol, a, a + N, [](int x) { return x % 10 == 0; }) == N / 10);
  CHECK(r::contains(pol, a, 999));
  CHECK(!r::contains(pol, a, N));

  // copy into a shorter output range: truncated to the output's size
  {
    auto res = r::copy(pol, a, r::subrange(b, b + 10));
    static_assert(std::is_same_v<decltype(res), r::copy_result<int*, int*>>);
    CHECK(res.in == a + 10 && res.out == b + 10);
    auto res2 = r::copy(pol, a, a + 5, c, c + N);  // input shorter: 5 copied
    CHECK(res2.in == a + 5 && res2.out == c + 5);
    for (int i = 0; i < 10; ++i) CHECK(b[i] == a[i]);
  }
  // transform: unary and binary, truncated to the shortest
  {
    auto u = r::transform(pol, a, r::subrange(b, b + 20), [](int x) { return -x; });
    static_assert(std::is_same_v<decltype(u), r::unary_transform_result<int*, int*>>);
    CHECK(u.in == a + 20 && u.out == b + 20 && b[1] == -37);
    auto u2 = r::transform(pol, a, a + N, b, b + N, [](int x) { return x * 2; }, [](int x) { return x + 1; });
    CHECK(u2.in == a + N && u2.out == b + N && b[1] == 76);
    auto bi = r::transform(pol, r::subrange(a, a + 30), b, c, std::plus<>());
    static_assert(std::is_same_v<decltype(bi), r::binary_transform_result<int*, int*, int*>>);
    CHECK(bi.in1 == a + 30 && bi.in2 == b + 30 && bi.out == c + 30);
    CHECK(c[1] == 37 + 76);
  }

  static_assert(std::is_same_v<decltype(r::fill(pol, b, 0)), int*>);
  CHECK(r::fill(pol, b, 5) == b + N);
  CHECK(r::count(pol, b, 5) == N);
  CHECK(r::equal(pol, b, b + N, b, b + N));
  CHECK(!r::equal(pol, a, b));
  CHECK(!r::equal(pol, r::subrange(b, b + 3), b));  // different lengths
  {
    r::copy(a, b);
    b[400] = -1;
    auto m = r::mismatch(pol, a, b);
    static_assert(std::is_same_v<decltype(m), r::mismatch_result<int*, int*>>);
    CHECK(m.in1 == a + 400 && m.in2 == b + 400);
  }
  {
    r::copy(a, b);
    static_assert(std::is_same_v<decltype(r::sort(pol, b)), int*>);
    CHECK(r::sort(pol, b) == b + N);
    for (int i = 0; i < N; ++i) CHECK(b[i] == i);
    CHECK(r::sort(pol, b, b + N, r::greater{}) == b + N);
    CHECK(b[0] == N - 1);
    CHECK(r::reverse(pol, b) == b + N);
    CHECK(b[0] == 0);
    CHECK(r::is_sorted(pol, b));
    CHECK(r::min_element(pol, a) == a);
    CHECK(*r::max_element(pol, a, a + N) == N - 1);
  }
  {
    static Rec recs[N];
    for (int i = 0; i < N; ++i) recs[i] = Rec{a[i] % 5, i};
    CHECK(r::stable_sort(pol, recs, {}, &Rec::key) == recs + N);
    for (int i = 1; i < N; ++i)
      CHECK(recs[i - 1].key < recs[i].key || (recs[i - 1].key == recs[i].key && recs[i - 1].id < recs[i].id));
  }
  {
    for (int i = 0; i < N; ++i) b[i] = i % 4;
    auto rem = r::remove(pol, b, 0);
    static_assert(std::is_same_v<decltype(rem), r::subrange<int*>>);
    CHECK(rem.begin() == b + 3 * N / 4 && rem.end() == b + N);
    for (int* q = b; q != rem.begin(); ++q) CHECK(*q != 0);
    CHECK(b[0] == 1 && b[1] == 2 && b[2] == 3 && b[3] == 1);
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
