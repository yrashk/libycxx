// [alg.req.sortable], [alg.req.permutable]: the ranges sorting algorithms use
// ranges::iter_move and ranges::iter_swap, so they work on ranges with proxy references
// such as views::zip ([range.zip]: zip_view's iterator customizes iter_move/iter_swap and
// its reference is a tuple of references). Sorting a zip of two arrays permutes both.
#include <algorithm>
#include <functional>
#include <ranges>
#include <tuple>
#include "check.hpp"

constexpr bool test() {
  {
    int k[] = {3, 1, 2, 5, 4};
    char v[] = {'c', 'a', 'b', 'e', 'd'};
    auto z = std::views::zip(k, v);
    std::ranges::sort(z);
    for (int i = 0; i < 5; ++i)
      if (k[i] != i + 1 || v[i] != 'a' + i) return false;
  }
  {
    // stable_sort by a projection onto the first element
    int k[] = {2, 1, 2, 1};
    int id[] = {0, 1, 2, 3};
    auto z = std::views::zip(k, id);
    std::ranges::stable_sort(z, {}, [](const auto& t) { return std::get<0>(t); });
    if (id[0] != 1 || id[1] != 3 || id[2] != 0 || id[3] != 2) return false;
  }
  {
    int k[] = {9, 3, 7, 1, 5};
    int w[] = {90, 30, 70, 10, 50};
    auto z = std::views::zip(k, w);
    std::ranges::nth_element(z, z.begin() + 2);
    if (k[2] != 5 || w[2] != 50) return false;
    std::ranges::make_heap(z);
    if (k[0] != 9 || w[0] != 90) return false;
    std::ranges::sort_heap(z);
    for (int i = 0; i < 5; ++i)
      if (w[i] != 10 * k[i]) return false;
    auto p = std::ranges::partition(z, [](const auto& t) { return std::get<0>(t) > 4; });
    if (p.begin() != z.begin() + 3) return false;
    for (int i = 0; i < 5; ++i)
      if (w[i] != 10 * k[i]) return false;
    std::ranges::sort(z, std::ranges::greater{});
    if (k[0] != 9 || w[4] != 10) return false;
    if (!std::ranges::next_permutation(z, std::ranges::greater{}).found) return false;
    for (int i = 0; i < 5; ++i)
      if (w[i] != 10 * k[i]) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
