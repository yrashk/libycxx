// The ranges algorithms work through ranges::iter_move / ranges::iter_swap and the
// indirectly_* concepts, so they accept proxy references ([alg.req.permutable],
// [alg.req.sortable]). vector<bool>::reference is such a proxy: [vector.bool.pspc] gives it
// "constexpr const reference& operator=(bool x) const noexcept;" (so the iterator is
// indirectly_writable) and swap; zip_view's iterator ([range.zip.iterator]) has a tuple of
// the underlying references as its reference, iter_move gives a tuple of iter_move results
// and iter_swap swaps element-wise. Every algorithm below permutes a zip of a vector<bool>
// and an int sequence; with the ints all distinct the results are fully determined for the
// stable algorithms ([alg.sort] stable_sort, [alg.partitions] stable_partition, [alg.merge]
// inplace_merge: "Remarks: Stable") and checked by content for the others.
#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <ranges>
#include <tuple>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using VB = std::vector<bool>;
static_assert(rg::random_access_range<VB>);
static_assert(std::sortable<VB::iterator>);
static_assert(std::permutable<rg::iterator_t<decltype(std::views::zip(std::declval<VB&>(), std::declval<std::vector<int>&>()))>>);

static VB pattern(int n) {
  VB b;
  for (int i = 0; i < n; ++i) b.push_back(((i * 7) % 5) < 2);
  return b;
}

// Checks that (b, v) is a permutation of the original pairs (bit i, i).
static bool same_pairs(const VB& b, const std::vector<int>& v, const VB& orig) {
  std::vector<bool> seen(v.size());
  for (std::size_t i = 0; i < v.size(); ++i) {
    int k = v[i];
    if (k < 0 || static_cast<std::size_t>(k) >= v.size() || seen[k] || b[i] != orig[k]) return false;
    seen[k] = true;
  }
  return true;
}

int main() {
  const int n = 37;
  const VB orig = pattern(n);
  std::vector<int> ids(n);
  std::iota(ids.begin(), ids.end(), 0);

  auto expect_stable_by_bit = [&](bool falses_first) {
    std::vector<int> e;
    for (int pass = 0; pass < 2; ++pass)
      for (int i = 0; i < n; ++i)
        if (orig[i] == (pass == 0 ? !falses_first : falses_first)) e.push_back(i);
    return e;
  };
  const auto proj0 = [](const auto& t) -> bool { return std::get<0>(t); };

  {  // stable_sort by the bit
    VB b = orig;
    std::vector<int> v = ids;
    auto z = std::views::zip(b, v);
    auto r = rg::stable_sort(z, {}, proj0);
    CHECK(r == z.end());
    CHECK(v == expect_stable_by_bit(true));
    CHECK(same_pairs(b, v, orig) && rg::is_sorted(b));
    rg::stable_sort(z, rg::greater{}, proj0);
    CHECK(v == expect_stable_by_bit(false));
  }
  {  // stable_partition
    VB b = orig;
    std::vector<int> v = ids;
    auto z = std::views::zip(b, v);
    auto r = rg::stable_partition(z, proj0);
    CHECK(r.begin() - z.begin() == rg::count(orig, true) && r.end() == z.end());
    CHECK(v == expect_stable_by_bit(false));
  }
  {  // inplace_merge of two bit-sorted halves
    VB b = orig;
    std::vector<int> v = ids;
    auto z = std::views::zip(b, v);
    auto mid = z.begin() + n / 2;
    rg::stable_sort(z.begin(), mid, {}, proj0);
    rg::stable_sort(mid, z.end(), {}, proj0);
    std::vector<int> e;
    for (int bit = 0; bit < 2; ++bit) {
      for (int i : v)
        if (orig[i] == bool(bit) && (std::find(v.begin(), v.begin() + n / 2, i) != v.begin() + n / 2)) e.push_back(i);
      for (int i : v)
        if (orig[i] == bool(bit) && (std::find(v.begin() + n / 2, v.end(), i) != v.end())) e.push_back(i);
    }
    CHECK(rg::inplace_merge(z, mid, {}, proj0) == z.end());
    CHECK(v == e);
  }
  {  // unstable algorithms: content checks
    VB b = orig;
    std::vector<int> v = ids;
    auto z = std::views::zip(b, v);
    rg::sort(z);  // tuple<bool, int> order
    CHECK(same_pairs(b, v, orig) && rg::is_sorted(z));
    rg::reverse(z);
    CHECK(rg::is_sorted(z, rg::greater{}));
    std::mt19937 g(3);
    rg::shuffle(z, g);
    CHECK(same_pairs(b, v, orig));
    auto mid = z.begin() + 10;
    rg::partial_sort(z, mid);
    CHECK(same_pairs(b, v, orig) && rg::is_sorted(z.begin(), mid));
    for (auto it = mid; it != z.end(); ++it) CHECK(!(*it < *(mid - 1)));
    rg::shuffle(z, g);
    rg::nth_element(z, z.begin() + 20);
    for (auto it = z.begin(); it != z.begin() + 20; ++it) CHECK(!(z.begin()[20] < *it));
    rg::make_heap(z);
    CHECK(rg::is_heap(z));
    rg::sort_heap(z);
    CHECK(rg::is_sorted(z) && same_pairs(b, v, orig));
    const std::vector<int> before = v;
    auto rot = rg::rotate(z, z.begin() + 5);
    CHECK(rot.begin() == z.begin() + (n - 5) && rot.end() == z.end());
    CHECK(rg::equal(v.begin(), v.end() - 5, before.begin() + 5, before.end()) &&
          rg::equal(v.end() - 5, v.end(), before.begin(), before.begin() + 5));
    CHECK(same_pairs(b, v, orig));
    auto pr = rg::partition(z, proj0);
    CHECK(rg::is_partitioned(z, proj0) && pr.begin() - z.begin() == rg::count(orig, true));
  }
  {  // plain vector<bool>
    VB b = orig;
    rg::sort(b);
    CHECK(rg::is_sorted(b) && rg::count(b, true) == rg::count(orig, true));
    VB c = orig;
    rg::stable_sort(c, rg::greater{});
    CHECK(rg::is_sorted(c, rg::greater{}) && c.front() && !c.back());
    VB d = orig;
    auto u = rg::unique(d);
    d.erase(u.begin(), u.end());
    std::vector<char> m(orig.begin(), orig.end());
    m.erase(std::unique(m.begin(), m.end()), m.end());
    CHECK(rg::equal(d, m, [](bool x, char y) { return x == bool(y); }));
    VB e = orig;
    auto rm = rg::remove(e, true);
    CHECK(rm.begin() - e.begin() == rg::count(orig, false) && rg::none_of(e.begin(), rm.begin(), std::identity{}));
    VB f = orig;
    CHECK(rg::next_permutation(f).found);
    VB g2 = orig;
    rg::copy_backward(g2.begin(), g2.end() - 3, g2.end());  // overlapping, to the right
    CHECK(rg::equal(g2.begin() + 3, g2.end(), orig.begin(), orig.end() - 3));
    VB h = orig;
    rg::copy(h.begin() + 3, h.end(), h.begin());  // overlapping, to the left
    CHECK(rg::equal(h.begin(), h.end() - 3, orig.begin() + 3, orig.end()));
    VB s1 = orig, s2(n, true);
    rg::swap_ranges(s1, s2);
    CHECK(rg::all_of(s1, std::identity{}) && s2 == orig);
    rg::iter_swap(s2.begin(), s2.begin() + 1);
    CHECK(s2[0] == orig[1] && s2[1] == orig[0]);
    bool moved = rg::iter_move(s2.begin());
    CHECK(moved == orig[1]);
  }
  return 0;
}
