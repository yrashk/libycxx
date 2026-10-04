// [move.sentinel], [move.sent.ops], [move.iter.op.comp], [move.iter.nonmember]:
// move_sentinel<S> wraps a semiregular sentinel; move_iterator<I> == move_sentinel<S> when
// sentinel_for<S, I>; differences when sized_sentinel_for<S, I>; "move_sentinel<S> and
// move_iterator<I> model sentinel_for<move_sentinel<S>, move_iterator<I>>".
#include <iterator>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct End {
  const int* e;
  friend constexpr bool operator==(const int* p, End s) { return p == s.e; }
  friend constexpr std::ptrdiff_t operator-(End s, const int* p) { return s.e - p; }
  friend constexpr std::ptrdiff_t operator-(const int* p, End s) { return p - s.e; }
};

static_assert(std::sentinel_for<std::move_sentinel<End>, std::move_iterator<const int*>>);
static_assert(std::sized_sentinel_for<std::move_sentinel<End>, std::move_iterator<const int*>>);
static_assert(std::sentinel_for<std::move_sentinel<std::default_sentinel_t>, std::move_iterator<std::counted_iterator<int*>>>);
static_assert(std::is_same_v<decltype(std::move_sentinel<End>().base()), End>);

constexpr bool test() {
  const int a[3] = {1, 2, 3};
  std::move_iterator<const int*> it(a);
  std::move_sentinel<End> s(End{a + 3});
  int sum = 0;
  for (; it != s; ++it) sum += *it;
  if (sum != 6) return false;
  if (!(it == s)) return false;
  std::move_iterator<const int*> b(a);
  if (s - b != 3 || b - s != -3) return false;
  std::move_sentinel<const int*> ps(a + 1);
  std::move_sentinel<const int*> ps2;
  ps2 = ps;
  if (ps2.base() != a + 1) return false;
  std::move_sentinel<const int*> ps3 = std::move_sentinel<int*>(nullptr);  // converting
  if (ps3.base() != nullptr) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
