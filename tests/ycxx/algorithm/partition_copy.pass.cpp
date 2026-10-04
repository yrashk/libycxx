// [alg.partitions]: partition_copy "For each iterator i in [first, first + N), copies *i to
// the output range [out_true, last_true) if E(*i) is true, or to the output range
// [out_false, last_false) otherwise." Returns {out_true + Q, out_false + V} (std, a pair) and
// {first + N, out_true + Q, out_false + V} (ranges, partition_copy_result = in_out_out_result).
// "At most last - first applications of pred and proj". Input iterators suffice.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

struct P {
  int k;
};

constexpr bool test() {
  {
    int a[] = {1, 2, 3, 4, 5, 6, 7};
    int t[7] = {}, f[7] = {};
    int calls = 0;
    auto r = std::partition_copy(a, a + 7, t, f, [&calls](int x) {
      ++calls;
      return x % 3 == 0;
    });
    static_assert(std::is_same_v<decltype(r), std::pair<int*, int*>>);
    if (calls > 7) return false;
    if (r.first != t + 2 || r.second != f + 5) return false;
    if (t[0] != 3 || t[1] != 6) return false;
    if (f[0] != 1 || f[1] != 2 || f[2] != 4 || f[3] != 5 || f[4] != 7) return false;
  }
  {
    // input iterators; empty input
    int a[] = {4, 1, 2};
    int t[3] = {}, f[3] = {};
    auto r = std::partition_copy(InputIter<int>(a), InputIter<int>(a + 3), t, f, [](int x) { return x % 2 == 0; });
    if (r.first != t + 2 || r.second != f + 1 || t[0] != 4 || t[1] != 2 || f[0] != 1) return false;
    r = std::partition_copy(a, a, t, f, [](int) { return true; });
    if (r.first != t || r.second != f) return false;
  }
  {
    // ranges: projection and the in_out_out_result
    P ps[] = {{1}, {2}, {3}};
    P t[3], f[3];
    auto r = std::ranges::partition_copy(ps, t, f, [](int k) { return k > 1; }, &P::k);
    static_assert(std::is_same_v<decltype(r), std::ranges::partition_copy_result<P*, P*, P*>>);
    static_assert(std::is_same_v<std::ranges::partition_copy_result<P*, P*, P*>, std::ranges::in_out_out_result<P*, P*, P*>>);
    if (r.in != ps + 3 || r.out1 != t + 2 || r.out2 != f + 1) return false;
    if (t[0].k != 2 || t[1].k != 3 || f[0].k != 1) return false;
    int a[] = {9, 8, 7};
    int ti[3] = {}, fi[3] = {};
    InputRange<int> ir{a, a + 3};
    auto ri = std::ranges::partition_copy(ir, ti, fi, [](int x) { return x > 7; });
    if (ri.in.p != a + 3 || ri.out1 != ti + 2 || ri.out2 != fi + 1) return false;
    auto ri2 = std::ranges::partition_copy(InputIter<int>(a), PtrSentinel<int>{a + 3}, ti, fi, [](int x) { return x == 7; });
    if (ri2.in.p != a + 3 || ri2.out1 != ti + 1 || ti[0] != 7 || ri2.out2 != fi + 2) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
