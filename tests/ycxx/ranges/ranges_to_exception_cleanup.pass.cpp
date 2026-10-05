// ranges::to when the source range throws part-way (a filter predicate, a transform function,
// a generator body). [range.utility.conv.to]/2.1: for these containers and non-sized views
// ranges::to<C>(r) is C(from_range, r) (/2.1.2), or C(begin, end) / for_each(r,
// container-append(c)) (/2.1.3-4); in every case the exception propagates
// ([res.on.exception.handling]) and every element object that was constructed is destroyed
// exactly once (the partially built container is destroyed: [container.reqmts], [class.dtor];
// the prvalue produced by dereferencing a transform_view iterator is a temporary destroyed at the
// end of its full-expression: [class.temporary]/4). A live-object count must return to zero.
// [coro.generator.promise]/16: the generator body's exception is rethrown to the consumer; the
// generator's frame (and its local objects) is destroyed with the generator.
// Control: C(from_range, r) directly, with the same sources.
// REQUIRES: exceptions
#include <deque>
#include <generator>
#include <list>
#include <ranges>
#include <set>
#include <vector>
#include "check.hpp"

static int live = 0, budget = -1, failures = 0;
static void tick() {
  if (budget >= 0 && budget-- == 0) throw 42;
}
struct E {
  int v;
  E(int x) : v(x) { ++live; }
  E(const E& o) : v(o.v) { ++live; }
  E(E&& o) noexcept : v(o.v) { ++live; }
  E& operator=(const E&) = default;
  ~E() { --live; }
  friend auto operator<=>(const E&, const E&) = default;
};

std::generator<E> gen(int n) {
  E local(-1);  // destroyed with the frame
  for (int i = 0; i < n; ++i) {
    tick();
    co_yield E(i);
  }
}

template <class Make>
void sweep(Make make, int line) {
  bool completed = false;
  for (int limit = 0; limit < 200 && !completed; ++limit) {
    std::vector<E> in;
    for (int i = 0; i < 12; ++i) in.emplace_back(i);
    const int base = live;
    budget = limit;
    bool threw = false;
    try {
      make(in);
    } catch (int e) {
      CHECK(e == 42);
      threw = true;
    }
    budget = -1;
    completed = !threw;
    if (live != base) {  // report every failing source, then fail at the end
      dprintf(2, "line %d: limit %d: %d objects not destroyed\n", line, limit, live - base);
      ++failures;
      live = base;
      break;
    }
  }
  CHECK(completed || failures > 0);
}

auto keep = [](const E& e) {
  tick();
  return e.v % 3 != 0;
};
auto twice = [](const E& e) {
  tick();
  return E(e.v * 2);
};

int main() {
  // control: the from_range constructors
  sweep([](std::vector<E>& in) { std::vector<E> c(std::from_range, in | std::views::filter(keep) | std::views::transform(twice)); }, __LINE__);
  sweep([](std::vector<E>& in) { std::deque<E> c(std::from_range, in | std::views::filter(keep) | std::views::transform(twice)); }, __LINE__);
  // ranges::to
  sweep([](std::vector<E>& in) { auto c = in | std::views::filter(keep) | std::views::transform(twice) | std::ranges::to<std::vector>(); }, __LINE__);
  sweep([](std::vector<E>& in) { auto c = in | std::views::filter(keep) | std::views::transform(twice) | std::ranges::to<std::vector<E>>(); }, __LINE__);
  sweep([](std::vector<E>& in) { auto c = in | std::views::filter(keep) | std::views::transform(twice) | std::ranges::to<std::deque>(); }, __LINE__);
  sweep([](std::vector<E>& in) { auto c = in | std::views::filter(keep) | std::views::transform(twice) | std::ranges::to<std::list>(); }, __LINE__);
  sweep([](std::vector<E>& in) { auto c = in | std::views::filter(keep) | std::views::transform(twice) | std::ranges::to<std::set>(); }, __LINE__);
  sweep([](std::vector<E>& in) { auto c = in | std::views::transform(twice) | std::ranges::to<std::vector>(); }, __LINE__);
  sweep([](std::vector<E>& in) { auto c = in | std::views::filter(keep) | std::ranges::to<std::vector>(); }, __LINE__);
  // generator sources
  sweep([](std::vector<E>&) { auto c = gen(10) | std::ranges::to<std::vector>(); }, __LINE__);
  sweep([](std::vector<E>&) { auto c = gen(10) | std::views::transform(twice) | std::ranges::to<std::vector>(); }, __LINE__);
  sweep([](std::vector<E>&) { std::vector<E> c(std::from_range, gen(10)); }, __LINE__);
  CHECK(failures == 0);
  CHECK(live == 0);

  // completed results
  std::vector<E> in;
  for (int i = 0; i < 6; ++i) in.emplace_back(i);
  auto r = in | std::views::filter(keep) | std::views::transform(twice) | std::ranges::to<std::vector>();
  CHECK((r == std::vector<E>{2, 4, 8, 10}));
  CHECK((gen(3) | std::ranges::to<std::vector>()) == (std::vector<E>{0, 1, 2}));
  return 0;
}
