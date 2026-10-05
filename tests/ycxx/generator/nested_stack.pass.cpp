// [coro.generator.promise]/10-11: co_yield ranges::elements_of(g) accepts a generator<R2, V2,
// Alloc2> of any allocator type whose yielded type is the same ("requires
// same_as<typename generator<R2, V2, Alloc2>::yielded, yielded>"), and pushes its coroutine
// on the active stack: generators allocated with three different allocator types nest, and
// each coroutine frame is allocated and deallocated with its own allocator (/17-22).
// /13: for any other range (including a generator with a different yielded type, e.g.
// generator<const long&> inside generator<int>), elements_of(r, alloc) produces the elements
// through a nested generator<yielded, void, Alloc> "nested(allocator_arg, r.allocator, ...)":
// its frame is allocated with r.allocator; "co_yield static_cast<yielded>(*i)" -- an
// exception from that conversion is thrown inside the nested coroutine, stored in except_
// (/16: it is not the sole element of the stack) and rethrown by await_resume in the parent
// (/11), where the parent can catch it and continue.
// /16 at depth: a generator three levels down throws after yielding; the level that catches
// it continues; uncaught, the exception emerges from the iterator increment of the root, and
// the iterator then equals default_sentinel ([dcl.fct.def.coroutine]/14, see exceptions.pass).
// [coro.generator.members]/2 (~generator destroys coroutine_) with Note: "destroying a
// generator object destroys the whole stack" -- destroying the root while the active stack
// is several levels deep destroys every frame (each local destroyed exactly once) and returns
// every frame's memory to its allocator.
// REQUIRES: exceptions
#include <generator>
#include <cstddef>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <vector>
#include "check.hpp"

struct Stats {
  int allocs = 0, deallocs = 0;
};
Stats sa, sb, sc;

template <class T, Stats* S>
struct CA {  // default-constructible counting allocator
  using value_type = T;
  template <class U>
  struct rebind { using other = CA<U, S>; };
  CA() = default;
  template <class U>
  CA(const CA<U, S>&) noexcept {}
  T* allocate(std::size_t n) {
    ++S->allocs;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    ++S->deallocs;
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U>
  friend bool operator==(const CA&, const CA<U, S>&) noexcept { return true; }
};
template <class T> using AA = CA<T, &sa>;
template <class T> using AB = CA<T, &sb>;
template <class T> using AC = CA<T, &sc>;

static int live_guards = 0, guards_made = 0;
struct Guard {
  Guard() { ++live_guards; ++guards_made; }
  Guard(const Guard&) = delete;
  ~Guard() { --live_guards; }
};

std::generator<int, void, AC<int>> leaf(int base, int n, bool fail) {
  Guard g;
  for (int i = 0; i < n; ++i) co_yield base + i;
  if (fail) throw std::runtime_error("leaf");
}
std::generator<int, void, AB<int>> middle(int base, bool fail, bool catch_it) {
  Guard g;
  co_yield base;
  if (catch_it) {
    bool caught = false;
    try {
      co_yield std::ranges::elements_of(leaf(base + 1, 2, fail));
    } catch (const std::runtime_error&) {
      caught = true;  // (co_yield is not allowed in a handler)
    }
    if (caught) co_yield -1;
  } else {
    co_yield std::ranges::elements_of(leaf(base + 1, 2, fail));
  }
  co_yield base + 9;
}
std::generator<int, void, AA<int>> top(bool fail, bool catch_it) {
  Guard g;
  co_yield 0;
  co_yield std::ranges::elements_of(middle(10, false, catch_it));
  co_yield std::ranges::elements_of(middle(20, fail, catch_it));
  co_yield 99;
}
std::generator<int> root(bool fail, bool catch_it) {
  Guard g;
  co_yield std::ranges::elements_of(top(fail, catch_it));
  co_yield 100;
}

static void reset() { sa = sb = sc = Stats{}; guards_made = 0; }
static bool balanced() {
  return sa.allocs == sa.deallocs && sb.allocs == sb.deallocs && sc.allocs == sc.deallocs && live_guards == 0;
}

std::generator<const long&> crefs(const std::vector<long>& v) {
  for (const long& x : v) co_yield x;
}
struct Bad {
  int v;
  operator int() const {
    if (v < 0) throw std::runtime_error("convert");
    return v;
  }
};
std::generator<int> convert(const std::vector<Bad>& v) {
  bool caught = false;
  try {
    co_yield std::ranges::elements_of(v, AB<std::byte>());
  } catch (const std::runtime_error&) {
    caught = true;
  }
  if (caught) co_yield -100;
  co_yield 7;
}
std::generator<int> mixed(const std::vector<long>& v) {
  co_yield std::ranges::elements_of(crefs(v), AC<std::byte>());  // different yielded: /13
}

int main() {
  {  // Full iteration across three allocator types.
    reset();
    std::vector<int> got;
    for (int x : root(false, false)) got.push_back(x);
    std::vector<int> want{0, 10, 11, 12, 19, 20, 21, 22, 29, 99, 100};
    CHECK(got == want);
    CHECK(sa.allocs == 1 && sb.allocs == 2 && sc.allocs == 2);
    CHECK(balanced() && guards_made == 6);
  }
  {  // Caught two levels up from the leaf: middle continues.
    reset();
    std::vector<int> got;
    for (int x : root(true, true)) got.push_back(x);
    std::vector<int> want{0, 10, 11, 12, 19, 20, 21, 22, -1, 29, 99, 100};
    CHECK(got == want);
    CHECK(balanced());
  }
  {  // Uncaught: emerges from the root's ++, then the iterator is at the end.
    reset();
    std::vector<int> got;
    bool threw = false;
    {
      auto g = root(true, false);
      auto it = g.begin();
      try {
        for (; it != std::default_sentinel; ++it) got.push_back(*it);
      } catch (const std::runtime_error&) {
        threw = true;
      }
      CHECK(threw && it == std::default_sentinel);
      CHECK(live_guards == 0);  // every frame has finished
    }
    std::vector<int> want{0, 10, 11, 12, 19, 20, 21, 22};
    CHECK(got == want);
    CHECK(balanced());
  }
  {  // Destroyed while three levels deep.
    for (int stop = 0; stop < 11; ++stop) {
      reset();
      {
        auto g = root(false, false);
        int n = 0;
        for (auto it = g.begin(); it != std::default_sentinel && n < stop; ++it) ++n;
      }
      CHECK(balanced());
    }
    reset();
    {
      auto g = root(false, false);
      auto it = g.begin();
      for (int i = 0; i < 6; ++i) ++it;  // inside leaf(21, ...)
      CHECK(*it == 21 && live_guards == 4);
    }
    CHECK(balanced());
    reset();
    { auto g = root(false, false); }  // never started
    CHECK(balanced() && guards_made == 0);
  }
  {  // elements_of(range, alloc) with a different yielded type: nested frame from the allocator.
    reset();
    std::vector<long> v{3, 4, 5};
    std::vector<int> got;
    for (int x : mixed(v)) got.push_back(x);
    CHECK((got == std::vector<int>{3, 4, 5}));
    CHECK(sc.allocs == 1 && sc.deallocs == 1);
  }
  {  // The conversion throws inside the nested generator; the parent catches it.
    reset();
    std::vector<Bad> v{{1}, {2}, {-1}, {4}};
    std::vector<int> got;
    for (int x : convert(v)) got.push_back(x);
    CHECK((got == std::vector<int>{1, 2, -100, 7}));
    CHECK(sb.allocs == 1 && sb.deallocs == 1);
  }
  {  // Deep recursion through elements_of.
    struct R {
      static std::generator<int> down(int d) {
        if (d == 0) {
          co_yield 0;
          co_return;
        }
        co_yield d;
        co_yield std::ranges::elements_of(down(d - 1));
      }
    };
    long sum = 0;
    int count = 0;
    for (int x : R::down(3000)) {
      sum += x;
      ++count;
    }
    CHECK(count == 3001 && sum == 3000L * 3001 / 2);
  }
  return 0;
}
