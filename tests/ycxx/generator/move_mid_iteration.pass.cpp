// [coro.generator.members]/1-2: generator(generator&& other) takes other's coroutine_ and
// active_; "Iterators previously obtained from other are not invalidated; they become
// iterators into *this." /5-7: operator=(generator other) swaps coroutine_ and active_, with
// the same note. So a generator moved (or move-assigned) while its iterator is suspended
// inside nested co_yield elements_of(...) levels keeps producing the remaining elements --
// of the nested generators too -- through the old iterator; the generator previously held by
// the assignment target is destroyed with its whole stack (/3-4) when the by-value parameter
// is destroyed. A moved-from generator can be destroyed and assigned to.
#include <generator>
#include <ranges>
#include <utility>
#include <vector>
#include "check.hpp"

static int live = 0;
struct Guard {
  Guard() { ++live; }
  Guard(const Guard&) = delete;
  ~Guard() { --live; }
};

std::generator<int> leaf(int a, int n) {
  Guard g;
  for (int i = 0; i < n; ++i) co_yield a + i;
}
std::generator<int> mid(int a) {
  Guard g;
  co_yield a;
  co_yield std::ranges::elements_of(leaf(a + 1, 3));
  co_yield a + 9;
}
std::generator<int> top() {
  Guard g;
  co_yield std::ranges::elements_of(mid(10));
  co_yield std::ranges::elements_of(mid(20));
  co_yield 99;
}

int main() {
  const std::vector<int> want{10, 11, 12, 13, 19, 20, 21, 22, 23, 29, 99};
  {  // Move construction three levels deep.
    std::vector<int> got;
    auto g = top();
    auto it = g.begin();
    for (int i = 0; i < 2; ++i, ++it) got.push_back(*it);  // now inside leaf(11, 3)
    CHECK(*it == 12 && live == 3);
    auto moved = std::move(g);
    for (; it != moved.end(); ++it) got.push_back(*it);
    CHECK(got == want);
  }
  CHECK(live == 0);
  {  // Move assignment: the target's own partly consumed generator is destroyed.
    std::vector<int> got;
    auto a = top();
    auto ia = a.begin();
    ++ia;  // a is suspended inside leaf
    CHECK(live == 3);
    auto b = top();
    auto ib = b.begin();
    for (int i = 0; i < 6; ++i, ++ib) got.push_back(*ib);  // b inside leaf(21, 3)
    CHECK(live == 6);
    a = std::move(b);  // a's old stack (3 frames) is destroyed
    CHECK(live == 3);
    for (; ib != std::default_sentinel; ++ib) got.push_back(*ib);
    CHECK(got == want);
    b = top();  // assign to the moved-from generator
    std::vector<int> again;
    for (int x : b) again.push_back(x);
    CHECK(again == want);
  }
  CHECK(live == 0);
  {  // Swapping two generators suspended at different depths.
    auto a = top();
    auto b = top();
    auto ia = a.begin();
    auto ib = b.begin();
    for (int i = 0; i < 4; ++i) ++ib;  // b at 19 (mid level)
    std::swap(a, b);
    ++ia;
    ++ib;
    CHECK(*ia == 11 && *ib == 20);
  }
  CHECK(live == 0);
  return 0;
}
