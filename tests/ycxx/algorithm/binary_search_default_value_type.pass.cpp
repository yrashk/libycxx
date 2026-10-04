// [alg.binary.search]: the value parameter's type T defaults to iterator_traits<
// ForwardIterator>::value_type (std) and projected_value_t<I, Proj> (ranges), so the value
// can be a braced-init-list: lower_bound(first, last, {a, b}).
#include <algorithm>
#include <ranges>
#include "check.hpp"

struct Pt {
  int x, y;
  friend constexpr auto operator<=>(const Pt&, const Pt&) = default;
};
struct Box {
  Pt p;
};

constexpr bool test() {
  Pt ps[] = {{0, 1}, {1, 0}, {1, 2}, {2, 2}};
  if (std::lower_bound(ps, ps + 4, {1, 1}) != ps + 2) return false;
  if (std::upper_bound(ps, ps + 4, {1, 0}) != ps + 2) return false;
  auto er = std::equal_range(ps, ps + 4, {1, 2});
  if (er.first != ps + 2 || er.second != ps + 3) return false;
  if (!std::binary_search(ps, ps + 4, {2, 2})) return false;
  if (std::lower_bound(ps, ps + 4, {1, 1}, std::less<>{}) != ps + 2) return false;

  if (std::ranges::lower_bound(ps, {1, 1}) != ps + 2) return false;
  if (std::ranges::upper_bound(ps, ps + 4, {1, 0}) != ps + 2) return false;
  if (!std::ranges::binary_search(ps, {0, 1})) return false;
  if (std::ranges::equal_range(ps, {9, 9}).begin() != ps + 4) return false;
  // with a projection T is the projected value type
  Box bs[] = {{{0, 0}}, {{3, 3}}};
  if (std::ranges::lower_bound(bs, {3, 3}, {}, &Box::p) != bs + 1) return false;
  if (!std::ranges::binary_search(bs, {0, 0}, {}, &Box::p)) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
