// views::concat of four ranges of every combination of sizes 0, 1, 2 and 5 (empty ranges at the
// front, in the middle and at the back), checked against the flattened sequence.
// [range.concat.iterator]: for random-access concat-able ranges the iterator supports it + n,
// n + it, it - n, it += n, it -= n, it[n], it - it, the relational operators and <=>; ++ and --
// skip empty ranges; [range.concat.view]: size() is the sum of the sizes and end() is an
// iterator when the last range is common; the iterator - default_sentinel / sentinel distance is
// the number of remaining elements. Also with a const view, and mixing vector, iota_view and
// array whose common reference is a prvalue.
#include <array>
#include <compare>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;

template <class View>
void check_view(View& cv, const std::vector<int>& flat) {
  const long n = static_cast<long>(flat.size());
  static_assert(rg::random_access_range<View> && rg::sized_range<View> && rg::common_range<View>);
  CHECK(static_cast<long>(rg::size(cv)) == n);
  auto b = rg::begin(cv);
  auto e = rg::end(cv);
  CHECK(e - b == n && b - e == -n);
  CHECK((b == e) == (n == 0));
  // forward and backward walks
  {
    long i = 0;
    for (auto it = b; it != e; ++it, ++i) CHECK(*it == flat[static_cast<std::size_t>(i)]);
    CHECK(i == n);
    auto it = e;
    for (long k = n - 1; k >= 0; --k) {
      --it;
      CHECK(*it == flat[static_cast<std::size_t>(k)]);
    }
    CHECK(it == b);
    it = b;
    for (long k = 0; k < n; ++k) CHECK(*it++ == flat[static_cast<std::size_t>(k)]);
    CHECK(it == e);
  }
  // arithmetic between every pair of positions
  for (long i = 0; i <= n; ++i) {
    auto pi = b + i;
    CHECK(pi == i + b && pi == e - (n - i));
    CHECK(pi - b == i && e - pi == n - i);
    if (i < n) CHECK(b[i] == flat[static_cast<std::size_t>(i)] && *pi == flat[static_cast<std::size_t>(i)]);
    for (long j = 0; j <= n; ++j) {
      auto pj = b + j;
      CHECK(pj - pi == j - i);
      auto q = pi;
      q += j - i;
      CHECK(q == pj);
      q -= j - i;
      CHECK(q == pi);
      CHECK((pi < pj) == (i < j) && (pi <= pj) == (i <= j) && (pi > pj) == (i > j) && (pi >= pj) == (i >= j));
      CHECK((pi <=> pj) == (i <=> j));
      if (i < n) CHECK(pj[i - j] == flat[static_cast<std::size_t>(i)]);
    }
  }
}

int main() {
  const int sizes[] = {0, 1, 2, 5};
  int value = 0;
  for (int s0 : sizes)
    for (int s1 : sizes)
      for (int s2 : sizes)
        for (int s3 : sizes) {
          std::vector<int> v[4];
          std::vector<int> flat;
          const int ss[4] = {s0, s1, s2, s3};
          for (int r = 0; r < 4; ++r)
            for (int k = 0; k < ss[r]; ++k) {
              v[r].push_back(++value);
              flat.push_back(value);
            }
          auto c = std::views::concat(v[0], v[1], v[2], v[3]);
          check_view(c, flat);
          const auto& cc = c;
          check_view(cc, flat);
          // writes go through to the underlying vectors
          for (auto& x : c) x = -x;
          std::size_t k = 0;
          for (int r = 0; r < 4; ++r)
            for (int x : v[r]) CHECK(x == -flat[k++]);
          for (auto& x : c) x = -x;
          // a prvalue member: iota_view in the middle, array at the end
          auto iota = std::views::iota(1000, 1000 + s1);
          std::array<int, 2> arr{-1, -2};
          auto m = std::views::concat(v[0], iota, v[2], arr);
          std::vector<int> mflat(v[0].begin(), v[0].end());
          for (int x = 1000; x < 1000 + s1; ++x) mflat.push_back(x);
          mflat.insert(mflat.end(), v[2].begin(), v[2].end());
          mflat.insert(mflat.end(), arr.begin(), arr.end());
          static_assert(std::is_same_v<rg::range_reference_t<decltype(m)>, int>);
          check_view(m, mflat);
        }
}
