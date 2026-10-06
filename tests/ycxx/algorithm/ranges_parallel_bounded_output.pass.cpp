// The ranges:: parallel algorithm overloads (an execution-policy first parameter) write to a
// bounded output, [result, result_last) or result_r, and stop when it is full:
// [alg.copy]/7, /10.2: copy writes N = min(last - first, result_last - result) elements and
// returns {first + N, result + N}; /12-/14, /17.2: copy_n with M = max(0, n) and
// N = min(result_last - result, M) returns {first + N, result + N};
// /20, /23.2-/23.3: copy_if copies the first N = min(M, result_last - result) satisfying
// elements and returns {last, result + N} if N == M, otherwise {j, result_last}, j the first
// satisfying element not copied.
// [alg.move]: move the same as copy, moving.
// [alg.transform]/1.2-/1.4: N = min(M, result_last - result), M the input length (the shorter
// one for binary transforms); returns {first1 + N, result + N} or {first1 + N, first2 + N,
// result + N}.
// [alg.merge]/1.1.2, /1.4, /4.2: merge copies N = min(n1 + n2, result_last - result) elements,
// K of them from the first range, and returns {first1 + K, first2 + N - K, result + N}.
// [algorithms.parallel.defns]/2: an execution policy template parameter satisfies
// execution-policy, is_execution_policy_v<remove_cvref_t<Ep>> (so lvalue policies work).
#include <algorithm>
#include <execution>
#include <functional>
#include <memory>
#include <ranges>
#include <span>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;

template <class Pol>
void run(Pol&& pol) {
  const std::vector<int> in{1, 2, 3, 4, 5, 6};

  // copy: shorter output
  {
    std::vector<int> out(4, 0);
    auto r = rg::copy(pol, in, out);
    CHECK(r.in == in.begin() + 4 && r.out == out.end());
    CHECK((out == std::vector<int>{1, 2, 3, 4}));
    std::vector<int> big(8, 0);
    auto r2 = rg::copy(pol, in.begin(), in.end(), big.begin(), big.end());
    CHECK(r2.in == in.end() && r2.out == big.begin() + 6 && big[5] == 6 && big[6] == 0);
    std::vector<int> none;
    auto r3 = rg::copy(pol, in, none);
    CHECK(r3.in == in.begin() && r3.out == none.end());
  }
  // copy_n
  {
    std::vector<int> out(3, 0);
    auto r = rg::copy_n(pol, in.begin(), 5, out.begin(), out.end());
    CHECK(r.in == in.begin() + 3 && r.out == out.end() && out[2] == 3);
    auto r2 = rg::copy_n(pol, in.begin(), 2, out.begin(), out.end());
    CHECK(r2.in == in.begin() + 2 && r2.out == out.begin() + 2);
    auto r3 = rg::copy_n(pol, in.begin(), -4, out.begin(), out.end());  // M = max(0, n)
    CHECK(r3.in == in.begin() && r3.out == out.begin());
  }
  // copy_if: M = 3 even elements
  {
    auto even = [](int x) { return x % 2 == 0; };
    std::vector<int> out(2, 0);
    auto r = rg::copy_if(pol, in, out, even);
    CHECK(r.in == in.begin() + 5 && r.out == out.end());  // j: the third even element, 6
    CHECK(out[0] == 2 && out[1] == 4);
    std::vector<int> out3(3, 0);
    auto r2 = rg::copy_if(pol, in, out3, even);
    CHECK(r2.in == in.end() && r2.out == out3.end() && out3[2] == 6);
    std::vector<int> out5(5, -1);
    auto r3 = rg::copy_if(pol, in, out5, even, std::negate<>{});  // projection
    CHECK(r3.in == in.end() && r3.out == out5.begin() + 3 && out5[3] == -1);
  }
  // move
  {
    std::vector<std::unique_ptr<int>> src;
    for (int i = 0; i < 4; ++i) src.push_back(std::make_unique<int>(i));
    std::vector<std::unique_ptr<int>> dst(3);
    auto r = rg::move(pol, src, dst);
    CHECK(r.in == src.begin() + 3 && r.out == dst.end());
    CHECK(*dst[2] == 2 && !src[0] && src[3] && *src[3] == 3);
  }
  // transform, unary and binary
  {
    std::vector<int> out(4, 0);
    auto r = rg::transform(pol, in, out, [](int x) { return x * 10; });
    CHECK(r.in == in.begin() + 4 && r.out == out.end() && out[3] == 40);
    std::vector<int> b{100, 200, 300};
    std::vector<int> wide(10, 0);
    auto r2 = rg::transform(pol, in, b, wide, std::plus<>{});
    CHECK(r2.in1 == in.begin() + 3 && r2.in2 == b.end() && r2.out == wide.begin() + 3);
    CHECK(wide[2] == 303 && wide[3] == 0);
    std::vector<int> two(2, 0);
    auto r3 = rg::transform(pol, in.begin(), in.end(), b.begin(), b.end(), two.begin(), two.end(),
                            std::plus<>{}, std::negate<>{});
    CHECK(r3.in1 == in.begin() + 2 && r3.in2 == b.begin() + 2 && r3.out == two.end());
    CHECK(two[0] == 99 && two[1] == 198);
  }
  // merge
  {
    std::vector<int> a{1, 4, 6}, b{2, 3, 5, 7};
    std::vector<int> out(4, 0);
    auto r = rg::merge(pol, a, b, out);
    CHECK((out == std::vector<int>{1, 2, 3, 4}));
    CHECK(r.in1 == a.begin() + 2 && r.in2 == b.begin() + 2 && r.out == out.end());
    std::vector<int> all(9, 0);
    auto r2 = rg::merge(pol, a, b, all);
    CHECK(r2.in1 == a.end() && r2.in2 == b.end() && r2.out == all.begin() + 7 && all[6] == 7);
    // stability: from equal elements, those of the first range first
    struct P { int k, src; };
    std::vector<P> x{{1, 1}, {2, 1}}, y{{1, 2}, {2, 2}};
    std::vector<P> o(3);
    auto r3 = rg::merge(pol, x, y, o, {}, &P::k, &P::k);
    CHECK(o[0].src == 1 && o[1].src == 2 && o[2].k == 2 && o[2].src == 1);
    CHECK(r3.in1 == x.end() && r3.in2 == y.begin() + 1);
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  // borrowed results: an rvalue non-borrowed range gives dangling
  std::vector<int> out(3);
  auto r = rg::copy(std::execution::par, std::vector<int>{1, 2, 3}, std::span<int>(out));
  static_assert(std::is_same_v<decltype(r.in), rg::dangling>);
  static_assert(std::is_same_v<decltype(r.out), std::span<int>::iterator>);
  CHECK(out[2] == 3);
  return 0;
}
