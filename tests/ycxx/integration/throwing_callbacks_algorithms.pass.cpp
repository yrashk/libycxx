// Exceptions thrown by user callbacks (comparators, predicates, projections, transformations)
// from inside sequential algorithms and range adaptors. The draft does not require a sorting
// algorithm to leave a permutation behind when the comparator throws, but:
//   [res.on.exception.handling]/1-2 and [algorithms.requirements]: the exception the callback
//     throws propagates out of the algorithm (no algorithm here is noexcept; nothing swallows or
//     replaces the exception, and terminate is not called: [except.terminate] lists no such
//     case for sequential algorithms without an execution policy).
//   [basic.life], [class.dtor]: every object an algorithm creates (temporaries, buffer
//     elements: [alg.sort] stable_sort, inplace_merge, stable_partition may allocate one) is
//     destroyed exactly once, and no object is used after its destruction; afterwards every
//     element of the range is a valid object holding one of the original values or a
//     moved-from value ([lib.types.movedfrom]: valid but unspecified).
// Each callback throws on its n-th call for every n until the algorithm completes; the
// completed run is checked against the specified result.
// Also: [alg.foreach] for_each applies f "starting from first and proceeding to last - 1",
// so exactly k elements are visited before the throwing (k+1)-th call, through std::function
// ([func.wrap.func.inv]: invoking the target propagates its exception).
#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>
#include "check.hpp"

static int budget = -1;
static int calls = 0;
static void tick() {
  ++calls;
  if (budget >= 0 && budget-- == 0) throw std::string("cb");
}

static int live = 0;
struct E {
  int v;
  std::string s;  // makes moved-from states observable and allocates
  int magic = 0x5eed;
  E(int x) : v(x), s(std::string(20, char('a' + x % 26))) { ++live; }
  E(const E& o) : v(o.v), s(o.s) {
    CHECK(o.magic == 0x5eed);
    ++live;
  }
  E(E&& o) noexcept : v(o.v), s(std::move(o.s)) {
    CHECK(o.magic == 0x5eed);
    ++live;
  }
  E& operator=(const E& o) {
    CHECK(o.magic == 0x5eed && magic == 0x5eed);
    v = o.v;
    s = o.s;
    return *this;
  }
  E& operator=(E&& o) noexcept {
    CHECK(o.magic == 0x5eed && magic == 0x5eed);
    v = o.v;
    s = std::move(o.s);
    return *this;
  }
  ~E() {
    CHECK(magic == 0x5eed);
    magic = 0;
    --live;
  }
};
static bool less_v(const E& a, const E& b) {
  tick();
  CHECK(a.magic == 0x5eed && b.magic == 0x5eed);
  return a.v < b.v;
}

static std::vector<E> input() {
  std::vector<E> v;
  for (int i = 0; i < 40; ++i) v.emplace_back((i * 17 + 5) % 23);  // duplicates, unsorted
  return v;
}

static void check_elements(const std::vector<E>& v, const std::vector<E>& orig) {
  for (const E& e : v) {
    CHECK(e.magic == 0x5eed);
    bool from_input = std::ranges::any_of(orig, [&](const E& o) { return o.v == e.v; });
    CHECK(from_input);
    CHECK(e.s.empty() || e.s == std::string(20, char('a' + e.v % 26)));
  }
}

template <class Op, class Done>
void sweep(Op op, Done done) {
  const std::vector<E> orig = input();
  const int live0 = live;
  bool completed = false;
  for (int limit = 0; limit < 5000 && !completed; ++limit) {
    {
      std::vector<E> v = orig;
      budget = limit;
      bool threw = false;
      try {
        op(v);
      } catch (const std::string& s) {
        CHECK(s == "cb");
        threw = true;
      }
      budget = -1;
      completed = !threw;
      if (completed) done(v, orig);
      else check_elements(v, orig);
    }
    CHECK(live == live0);
  }
  CHECK(completed);
}

static auto by_v = [](const E& a, const E& b) { return a.v < b.v; };

int main() {
  auto sorted_check = [](const std::vector<E>& v, const std::vector<E>& orig) {
    CHECK(v.size() == orig.size());
    CHECK(std::ranges::is_sorted(v, by_v));
  };
  sweep([](std::vector<E>& v) { std::sort(v.begin(), v.end(), less_v); }, sorted_check);
  sweep([](std::vector<E>& v) { std::stable_sort(v.begin(), v.end(), less_v); }, sorted_check);
  sweep([](std::vector<E>& v) { std::ranges::sort(v, less_v); }, sorted_check);
  sweep([](std::vector<E>& v) { std::ranges::stable_sort(v, {}, [](const E& e) { tick(); return e.v; }); },
        sorted_check);
  sweep([](std::vector<E>& v) { std::partial_sort(v.begin(), v.begin() + 10, v.end(), less_v); },
        [](const std::vector<E>& v, const std::vector<E>&) { CHECK(std::is_sorted(v.begin(), v.begin() + 10, by_v)); });
  sweep([](std::vector<E>& v) { std::nth_element(v.begin(), v.begin() + 20, v.end(), less_v); },
        [](const std::vector<E>& v, const std::vector<E>&) {
          for (int i = 0; i < 20; ++i) CHECK(v[i].v <= v[20].v);
          for (int i = 21; i < 40; ++i) CHECK(v[i].v >= v[20].v);
        });
  sweep(
      [](std::vector<E>& v) {
        std::sort(v.begin(), v.begin() + 15, by_v);
        std::sort(v.begin() + 15, v.end(), by_v);
        std::inplace_merge(v.begin(), v.begin() + 15, v.end(), less_v);
      },
      sorted_check);
  sweep(
      [](std::vector<E>& v) {
        std::stable_partition(v.begin(), v.end(), [](const E& e) {
          tick();
          return e.v % 2 == 0;
        });
      },
      [](const std::vector<E>& v, const std::vector<E>& orig) {
        auto mid = std::ranges::find_if(v, [](const E& e) { return e.v % 2 != 0; });
        CHECK(std::all_of(mid, v.end(), [](const E& e) { return e.v % 2 != 0; }));
        std::vector<int> even;
        for (const E& e : orig)
          if (e.v % 2 == 0) even.push_back(e.v);
        CHECK(std::ranges::equal(even, std::ranges::subrange(v.begin(), mid), {}, {}, &E::v));
      });
  sweep(
      [](std::vector<E>& v) {
        std::make_heap(v.begin(), v.end(), less_v);
        std::sort_heap(v.begin(), v.end(), less_v);
      },
      sorted_check);
  sweep(
      [](std::vector<E>& v) {
        std::vector<E> out;
        std::ranges::sort(v, by_v);
        std::ranges::merge(v, std::vector<E>{E(3), E(22)}, std::back_inserter(out), less_v);
        v = std::move(out);
      },
      [](const std::vector<E>& v, const std::vector<E>&) {
        CHECK(v.size() == 42);
        CHECK(std::ranges::is_sorted(v, by_v));
      });
  sweep(
      [](std::vector<E>& v) {
        std::ranges::sort(v, by_v);
        auto r = std::ranges::unique(v, [](const E& a, const E& b) {
          tick();
          return a.v == b.v;
        });
        v.erase(r.begin(), r.end());
      },
      [](const std::vector<E>& v, const std::vector<E>&) {
        CHECK(v.size() == 23);
        CHECK(std::ranges::adjacent_find(v, {}, &E::v) == v.end());
      });
  // views: a throwing transform consumed by ranges::to; the partially built vector is destroyed.
  sweep(
      [](std::vector<E>& v) {
        v = v | std::views::filter([](const E& e) {
              tick();
              return e.v > 3;
            }) |
            std::views::transform([](const E& e) {
              tick();
              return E(e.v * 2);
            }) |
            std::ranges::to<std::vector>();
      },
      [](const std::vector<E>& v, const std::vector<E>&) {
        CHECK(std::ranges::all_of(v, [](const E& e) { return e.v > 6 && e.v % 2 == 0; }));
      });

  // for_each through std::function: exactly k elements are visited before the throwing call.
  for (int k = 0; k < 5; ++k) {
    std::vector<int> visited;
    std::function<void(int)> f = [&](int x) {
      if (static_cast<int>(visited.size()) == k) throw x;
      visited.push_back(x);
    };
    int thrown = -1;
    try {
      std::ranges::for_each(std::views::iota(10, 20), f);
    } catch (int x) {
      thrown = x;
    }
    CHECK(thrown == 10 + k);
    CHECK(std::ranges::equal(visited, std::views::iota(10, 10 + k)));
    visited.clear();
    try {
      const std::vector<int> five = {10, 11, 12, 13, 14};
      std::for_each(five.begin(), five.end(), f);
    } catch (int x) {
      thrown = x;
    }
    CHECK(thrown == 10 + k);
  }
  CHECK(live == 0);
  return 0;
}
