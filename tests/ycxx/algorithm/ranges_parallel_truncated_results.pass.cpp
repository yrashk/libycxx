// The ranges:: parallel algorithms with a bounded output that can be too small:
// [alg.remove]/9, /13: remove_copy(_if) copies the first N = min(M, result_last - result) kept
// elements; returns {last, result + N} if N == M, else {j, result_last} with j the first kept
// element not copied. [alg.unique]/7, /11: unique_copy likewise.
// [alg.replace]/7, /10: replace_copy(_if) writes N = min(last - first, result_last - result).
// [alg.reverse]/10, /12-/13: reverse_copy copies the last N elements [NEW_FIRST, last), reversed,
// and returns reverse_copy_truncated_result {last, NEW_FIRST, result + N}.
// [alg.rotate]/12, /14-/15: rotate_copy copies the first N elements of the rotated sequence and
// returns {middle + N, first, result + N} if N < last - middle, else
// {last, first + (N + (middle - first)) % M, result + N}.
// [alg.partitions]/16-/17, /20-/21: partition_copy with bounded outputs processes the first
// N = min(i1 - first, i2 - first) elements, and returns {first + N, out_true + Q, out_false + V}.
// [set.union]/1-/4: N = min(M, result_last - result) elements of the union; {last1, last2,
// result + N} if N == M, else {first1 + A, first2 + B, result_last}, A and B the copied or skipped
// elements. [set.difference]/3, /4.3: set_difference_truncated_result {first1 + A, first2 + B,
// result_last}; an element of the second range is skipped if it is less than the
// min(N + 1, M)th element of the difference.
#include <algorithm>
#include <execution>
#include <functional>
#include <type_traits>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using V = std::vector<int>;
using It = V::iterator;

template <class Pol>
void run(Pol&& pol) {
  // remove_copy / remove_copy_if
  {
    V in{1, 0, 2, 0, 3, 4};
    V out(2, -1);
    auto r = rg::remove_copy(pol, in, out, 0);
    CHECK(r.in == in.begin() + 4 && r.out == out.end() && out[0] == 1 && out[1] == 2);  // j -> 3
    V big(5, -1);
    auto r2 = rg::remove_copy_if(pol, in, big, [](int x) { return x == 0; });
    CHECK(r2.in == in.end() && r2.out == big.begin() + 4 && big[3] == 4 && big[4] == -1);
    V one(1, -1);
    auto r3 = rg::remove_copy(pol, in, one, -2, std::negate<>{});  // removes the 2
    CHECK(r3.in == in.begin() + 1 && r3.out == one.end() && one[0] == 1);
  }
  // unique_copy
  {
    V in{1, 1, 2, 2, 2, 3, 1};
    V out(3, 0);
    auto r = rg::unique_copy(pol, in, out);
    CHECK((out == V{1, 2, 3}) && r.in == in.begin() + 6 && r.out == out.end());
    V all(6, 0);
    auto r2 = rg::unique_copy(pol, in, all);
    CHECK(r2.in == in.end() && r2.out == all.begin() + 4 && all[3] == 1);
  }
  // replace_copy / replace_copy_if
  {
    V in{1, 2, 1, 3};
    V out(3, 0);
    auto r = rg::replace_copy(pol, in, out, 1, 9);
    CHECK((out == V{9, 2, 9}) && r.in == in.begin() + 3 && r.out == out.end());
    auto r2 = rg::replace_copy_if(pol, in.begin(), in.end(), out.begin(), out.begin() + 2,
                                  [](int x) { return x > 1; }, 0);
    CHECK(out[0] == 1 && out[1] == 0 && out[2] == 9 && r2.in == in.begin() + 2 && r2.out == out.begin() + 2);
  }
  // reverse_copy: the last N elements
  {
    V in{1, 2, 3, 4, 5};
    V out(2, 0);
    auto r = rg::reverse_copy(pol, in, out);
    static_assert(std::is_same_v<decltype(r), rg::reverse_copy_truncated_result<It, It>>);
    static_assert(std::is_same_v<rg::reverse_copy_truncated_result<It, It>, rg::in_in_out_result<It, It, It>>);
    CHECK(out[0] == 5 && out[1] == 4);
    CHECK(r.in1 == in.end() && r.in2 == in.begin() + 3 && r.out == out.end());
    V big(7, 0);
    auto r2 = rg::reverse_copy(pol, in, big);
    CHECK(r2.in1 == in.end() && r2.in2 == in.begin() && r2.out == big.begin() + 5 && big[4] == 1);
  }
  // rotate_copy
  {
    V in{1, 2, 3, 4, 5};  // rotated at 3: 3 4 5 1 2
    V out(2, 0);
    auto r = rg::rotate_copy(pol, in, in.begin() + 2, out);
    static_assert(std::is_same_v<decltype(r), rg::rotate_copy_truncated_result<It, It>>);
    CHECK(out[0] == 3 && out[1] == 4);
    CHECK(r.in1 == in.begin() + 4 && r.in2 == in.begin() && r.out == out.end());  // (15.1)
    V four(4, 0);
    auto r2 = rg::rotate_copy(pol, in, in.begin() + 2, four);
    CHECK((four == V{3, 4, 5, 1}));
    CHECK(r2.in1 == in.end() && r2.in2 == in.begin() + 1 && r2.out == four.end());  // (15.2)
    V all(6, 0);
    auto r3 = rg::rotate_copy(pol, in.begin(), in.begin() + 2, in.end(), all.begin(), all.end());
    CHECK(r3.in1 == in.end() && r3.in2 == in.begin() + 2 && r3.out == all.begin() + 5);
  }
  // partition_copy: stops when either output is full
  {
    V in{1, 2, 3, 4, 5, 6, 7};
    auto odd = [](int x) { return x % 2 != 0; };
    V t(2, 0), f(10, 0);
    auto r = rg::partition_copy(pol, in, t, f, odd);
    // i1: the third odd element (5) at index 4; i2: none (last); N = 4
    CHECK(r.in == in.begin() + 4 && r.out1 == t.end() && r.out2 == f.begin() + 2);
    CHECK(t[0] == 1 && t[1] == 3 && f[0] == 2 && f[1] == 4 && f[2] == 0);
    V t2(10, 0), f2(1, 0);
    auto r2 = rg::partition_copy(pol, in, t2, f2, odd);
    CHECK(r2.in == in.begin() + 3 && r2.out1 == t2.begin() + 2 && r2.out2 == f2.end());
  }
  // set_union
  {
    V a{1, 3, 5}, b{2, 3, 4};
    V out(3, 0);
    auto r = rg::set_union(pol, a, b, out);
    CHECK((out == V{1, 2, 3}));
    CHECK(r.in1 == a.begin() + 2 && r.in2 == b.begin() + 2 && r.out == out.end());
    V all(8, 0);
    auto r2 = rg::set_union(pol, a, b, all);
    CHECK(r2.in1 == a.end() && r2.in2 == b.end() && r2.out == all.begin() + 5);
  }
  // set_difference
  {
    V a{1, 2, 4, 6}, b{2, 3, 7};
    V out(2, 0);
    auto r = rg::set_difference(pol, a, b, out);
    static_assert(std::is_same_v<decltype(r), rg::set_difference_truncated_result<It, It, It>>);
    CHECK(out[0] == 1 && out[1] == 4);
    CHECK(r.in1 == a.begin() + 3 && r.in2 == b.begin() + 2 && r.out == out.end());
    V all(5, 0);
    auto r2 = rg::set_difference(pol, a, b, all);  // (4.3.1): B counts the skipped 2 and 3
    CHECK(r2.in1 == a.end() && r2.in2 == b.begin() + 2 && r2.out == all.begin() + 3 && all[2] == 6);
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
