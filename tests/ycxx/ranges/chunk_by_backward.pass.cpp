// [range.chunk.by.view]/4-6: find-next(current) is "ranges::next(ranges::adjacent_find(current,
// ranges::end(base_), not_fn(ref(*pred_))), 1, ranges::end(base_))"; find-prev(current) is
// "reverse_view rv(subrange(ranges::begin(base_), current)); ... ranges::prev(ranges::
// adjacent_find(rv, not_fn(bind(ref(*pred_), placeholders::_2, placeholders::_1))).base(), 1,
// current)" -- the predicate is always called as pred(earlier, later), also when walking
// backwards. [range.chunk.by.iter]/7-8: operator-- sets next_ = current_ and current_ =
// find-prev(next_). With a non-symmetric predicate the chunks seen backwards (through
// views::reverse or --) must be the forward chunks in reverse order.
#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <ranges>
#include <vector>
#include "check.hpp"

template <class R>
static std::vector<std::vector<int>> collect(R&& r) {
  std::vector<std::vector<int>> out;
  for (auto&& chunk : r) out.emplace_back(chunk.begin(), chunk.end());
  return out;
}

int main() {
  const auto consecutive = [](int a, int b) { return b == a + 1; };
  using VV = std::vector<std::vector<int>>;
  {
    std::list<int> l = {1, 2, 3, 5, 6, 9, 10, 11, 12, 0};
    auto v = l | std::views::chunk_by(consecutive);
    const VV fwd = {{1, 2, 3}, {5, 6}, {9, 10, 11, 12}, {0}};
    CHECK(collect(v) == fwd);
    VV back = collect(v | std::views::reverse);
    std::reverse(back.begin(), back.end());
    CHECK(back == fwd);
    auto it = v.end();
    --it;
    CHECK(std::ranges::equal(*it, std::vector<int>{0}));
    --it;
    CHECK(std::ranges::equal(*it, std::vector<int>{9, 10, 11, 12}));
    it--;
    CHECK(std::ranges::equal(*it, std::vector<int>{5, 6}));
    ++it;
    CHECK(std::ranges::equal(*it, std::vector<int>{9, 10, 11, 12}));
    --it;
    --it;
    CHECK(it == v.begin());
  }
  {
    // less_equal: "runs" with equal neighbours; strictly increasing check only one way.
    std::vector<int> a = {1, 2, 2, 3, 0, 4, 5, 2, 2};
    auto v = a | std::views::chunk_by(std::ranges::less_equal{});
    const VV fwd = {{1, 2, 2, 3}, {0, 4, 5}, {2, 2}};
    CHECK(collect(v) == fwd);
    VV back = collect(v | std::views::reverse);
    std::reverse(back.begin(), back.end());
    CHECK(back == fwd);
    auto g = a | std::views::chunk_by(std::ranges::greater{});
    const VV gf = {{1}, {2}, {2}, {3, 0}, {4}, {5, 2}, {2}};
    CHECK(collect(g) == gf);
    VV gb = collect(g | std::views::reverse);
    std::reverse(gb.begin(), gb.end());
    CHECK(gb == gf);
  }
  {
    // A single chunk and single-element chunks.
    std::vector<int> one = {4, 5, 6};
    auto v = one | std::views::chunk_by(consecutive);
    CHECK(std::ranges::distance(v) == 1);
    CHECK(std::ranges::equal(*std::ranges::prev(v.end()), one));
    std::vector<int> sep = {5, 4, 3};
    auto w = sep | std::views::chunk_by(consecutive);
    CHECK((collect(w | std::views::reverse) == VV{{3}, {4}, {5}}));
    std::vector<int> none;
    CHECK(std::ranges::empty(none | std::views::chunk_by(consecutive)));
  }
  return 0;
}
