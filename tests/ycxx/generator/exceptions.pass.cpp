// [coro.generator.promise]/16: unhandled_exception() rethrows (throw;) when the coroutine is the
// only element of the active stack, otherwise stores current_exception() in except_;
// [coro.generator.promise]/11: the awaitable of co_yield elements_of(g) rethrows except_ in
// await_resume, i.e. the exception of a nested generator emerges from the co_yield in the parent,
// where it can be caught, and the parent can continue.
// [dcl.fct.def.coroutine]/14: if unhandled_exception() exits via an exception the coroutine is
// considered suspended at its final suspend point, so the iterator then equals default_sentinel.
#include <generator>
#include <ranges>
#include <stdexcept>
#include <vector>
#include "check.hpp"

struct E {
  int code;
};

std::generator<int> throws_after(int n) {
  for (int i = 0; i < n; ++i) co_yield i;
  throw E{n};
}

std::generator<int> throws_at_start() {
  throw E{-1};
  co_yield 0;
}

std::generator<int> catches_nested() {
  co_yield 100;
  int code = 0;
  try {
    co_yield std::ranges::elements_of(throws_after(2));
  } catch (const E& e) {  // ([expr.await]/2: no co_yield inside a handler)
    code = e.code;
  }
  co_yield 1000 + code;
  co_yield 200;
}

std::generator<int> propagates_nested() {
  co_yield 100;
  co_yield std::ranges::elements_of(throws_after(1));
  co_yield 200;  // not reached
}

std::generator<int> two_levels() {
  int code = 0;
  try {
    co_yield std::ranges::elements_of(propagates_nested());
  } catch (const E& e) {
    code = e.code;
  }
  co_yield 5000 + code;
}

// Exception from iterating a non-generator range in elements_of.
struct ThrowingView : std::ranges::view_interface<ThrowingView> {
  struct iterator {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    int i = 0;
    int operator*() const {
      if (i == 2) throw E{42};
      return i;
    }
    iterator& operator++() {
      ++i;
      return *this;
    }
    void operator++(int) { ++i; }
    bool operator==(std::default_sentinel_t) const { return i == 5; }
  };
  iterator begin() const { return {}; }
  std::default_sentinel_t end() const { return {}; }
};
static_assert(std::ranges::input_range<ThrowingView>);

std::generator<int> from_throwing_range() {
  int code = 0;
  try {
    co_yield std::ranges::elements_of(ThrowingView{});
  } catch (const E& e) {
    code = e.code;
  }
  co_yield code;
}

template <class G>
std::vector<int> collect(G&& g) {
  std::vector<int> out;
  for (int x : g) out.push_back(x);
  return out;
}

int main() {
  {
    auto g = throws_after(2);
    auto it = g.begin();
    CHECK(*it == 0);
    ++it;
    CHECK(*it == 1);
    bool caught = false;
    try {
      ++it;
    } catch (const E& e) {
      caught = e.code == 2;
    }
    CHECK(caught);
    CHECK(it == g.end());  // suspended at the final suspend point
  }
  {
    auto g = throws_at_start();
    bool caught = false;
    try {
      auto it = g.begin();
      (void)it;
    } catch (const E& e) {
      caught = e.code == -1;
    }
    CHECK(caught);
  }
  CHECK((collect(catches_nested()) == std::vector<int>{100, 0, 1, 1002, 200}));
  {
    std::vector<int> out;
    bool caught = false;
    try {
      for (int x : propagates_nested()) out.push_back(x);
    } catch (const E& e) {
      caught = e.code == 1;
    }
    CHECK(caught);
    CHECK((out == std::vector<int>{100, 0}));
  }
  CHECK((collect(two_levels()) == std::vector<int>{100, 0, 5001}));
  CHECK((collect(from_throwing_range()) == std::vector<int>{0, 1, 42}));
  return 0;
}
