// [coroutine.generator.overview]/2: the elements are produced by resuming the coroutine.
// [coro.generator.promise]: initial_suspend returns suspend_always (nothing runs before begin());
// [coro.generator.members]/9: begin() resumes the coroutine; /12 end() returns default_sentinel;
// [coro.generator.iterator]/5 operator* is static_cast<reference>(*p.value_), /7 ++ resumes,
// /10 == default_sentinel is coroutine_.done().
// [coro.generator.promise]/4: yield_value(yielded) stores the address of the operand (no copy);
// /7: co_yield of an lvalue with an rvalue-reference yielded type stores a copy (the yielded
// object is not modified).
// [coro.generator.members]/1-7: move construction and assignment transfer the coroutine; existing
// iterators become iterators into the new generator. /3: the destructor destroys the coroutine
// (its automatic objects are destroyed).
#include <generator>
#include <ranges>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"

int started = 0;
std::generator<int> iota(int n) {
  ++started;
  for (int i = 0; i < n; ++i) co_yield i;
}

std::generator<int> ints(int start = 0) {
  while (true) co_yield start++;
}

struct Tracker {
  int copies = 0, moves = 0;
  Tracker() = default;
  Tracker(const Tracker& o) : copies(o.copies + 1), moves(o.moves) {}
  Tracker(Tracker&& o) noexcept : copies(o.copies), moves(o.moves + 1) {}
};

Tracker* yielded_address = nullptr;
std::generator<Tracker> lvalue_yield(Tracker& t) {
  co_yield t;  // const& overload: a copy is yielded
  co_yield std::move(t);  // yielded = Tracker&&: no copy, the address of t itself
}

std::generator<int&> refs(std::vector<int>& v) {
  for (int& x : v) co_yield x;
}

int alive = 0;
struct Guard {
  Guard() { ++alive; }
  ~Guard() { --alive; }
};
std::generator<int> guarded() {
  Guard g;
  co_yield 1;
  co_yield 2;
}

std::generator<std::string_view, std::string> views_of() {
  co_yield "ab";
  co_yield std::string_view("cd");
}

int main() {
  {
    started = 0;
    auto g = iota(3);
    CHECK(started == 0);  // lazily started
    auto it = g.begin();
    CHECK(started == 1);
    CHECK(it != g.end());
    CHECK(*it == 0);
    ++it;
    CHECK(*it == 1);
    it++;
    CHECK(*it == 2);
    ++it;
    CHECK(it == std::default_sentinel);
    CHECK(it == g.end() && g.end() == it);
  }
  {
    std::vector<int> out;
    for (int x : iota(4)) out.push_back(x);
    CHECK((out == std::vector<int>{0, 1, 2, 3}));
    out.clear();
    for (int x : iota(0)) out.push_back(x);
    CHECK(out.empty());
  }
  {
    // [coroutine.generator.overview] Example 1
    std::vector<int> out;
    for (int i : ints() | std::views::take(3)) out.push_back(i);
    CHECK((out == std::vector<int>{0, 1, 2}));
    auto out2 = iota(5) | std::views::transform([](int x) { return x * x; }) | std::ranges::to<std::vector>();
    CHECK((out2 == std::vector<int>{0, 1, 4, 9, 16}));
  }
  {
    Tracker t;
    auto g = lvalue_yield(t);
    auto it = g.begin();
    Tracker&& r = *it;
    CHECK(&r != &t);  // a copy
    CHECK(r.copies == 1 && t.copies == 0 && t.moves == 0);
    ++it;
    Tracker&& r2 = *it;
    CHECK(&r2 == &t);  // the object itself
    Tracker moved = *it;  // operator* returns Tracker&&: moved from
    CHECK(moved.moves == 1 && moved.copies == 0);
    ++it;
    CHECK(it == g.end());
  }
  {
    std::vector<int> v{1, 2, 3};
    for (int& x : refs(v)) x *= 10;
    CHECK((v == std::vector<int>{10, 20, 30}));
    auto g = refs(v);
    auto it = g.begin();
    CHECK(&*it == &v[0]);
  }
  {
    std::vector<std::string> out;
    for (std::string_view s : views_of()) out.emplace_back(s);
    CHECK(out.size() == 2 && out[0] == "ab" && out[1] == "cd");
  }
  {
    // Move construction / assignment keep iterators valid.
    auto g1 = iota(5);
    auto it = g1.begin();
    ++it;
    std::generator<int> g2(std::move(g1));
    CHECK(*it == 1);
    ++it;
    CHECK(*it == 2);
    auto g3 = iota(1);
    g3 = std::move(g2);
    ++it;
    CHECK(*it == 3);
    ++it;
    ++it;
    CHECK(it == g3.end());
    // iterator move operations
    auto g4 = iota(3);
    auto i1 = g4.begin();
    auto i2 = std::move(i1);
    CHECK(*i2 == 0);
    ++i2;
    decltype(i2) i3 = std::move(i2);
    CHECK(*i3 == 1);
    i2 = std::move(i3);
    CHECK(*i2 == 1);
  }
  {
    // Destroying a suspended generator destroys the coroutine's automatic objects.
    alive = 0;
    {
      auto g = guarded();
      CHECK(alive == 0);
      auto it = g.begin();
      CHECK(alive == 1 && *it == 1);
    }
    CHECK(alive == 0);
    {
      auto g = guarded();  // never started
    }
    CHECK(alive == 0);
    {
      auto g = guarded();
      for (int x : g) (void)x;
      CHECK(alive == 0);  // ran to completion
    }
  }
  return 0;
}
