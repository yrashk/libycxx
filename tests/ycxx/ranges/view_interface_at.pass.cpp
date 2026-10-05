// [view.interface.members]/5-6: at(n) "Returns: (*this)[n]. Throws: out_of_range if n < 0
// is true or n >= ranges::distance(derived()) is true." It requires a sized random-access
// range.
// REQUIRES: exceptions
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

template <class It, class Sent = It>
struct V : rg::view_interface<V<It, Sent>> {
  It b{};
  Sent e{};
  V() = default;
  constexpr V(It x, Sent y) : b(x), e(y) {}
  constexpr It begin() const { return b; }
  constexpr Sent end() const { return e; }
};

template <class T>
concept has_at = requires(T& t) { t.at(0); };
static_assert(has_at<V<int*>> && has_at<const V<int*>>);
static_assert(!has_at<V<RandomIter<int>, PtrSentinel<int>>>); // not sized
static_assert(!has_at<V<BidiIter<int>>>);
static_assert(std::is_same_v<decltype(std::declval<V<int*>&>().at(0)), int&>);

int main() {
  int a[3] = {7, 8, 9};
  V<int*> v(a, a + 3);
  CHECK(v.at(0) == 7 && v.at(2) == 9 && &v.at(1) == &a[1]);
  bool threw = false;
  try {
    (void)v.at(3);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);
  threw = false;
  try {
    (void)v.at(-1);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);
  // iota_view and other standard views inherit it.
  auto io = std::views::iota(10, 13);
  CHECK(io.at(2) == 12);
}
