// co_yield of an rvalue (or, for a reference `yielded`, of any matching glvalue) hands the
// consumer a reference to the very object the coroutine yielded: no copy and no move.
//   [coro.generator.promise]/4: yield_value(yielded val) noexcept: "Effects: Equivalent to
//     value_ = addressof(val)"; [coro.generator.iterator] operator*: "return
//     static_cast<reference>(*p.value_)". For generator<T> (T not a reference) reference and
//     yielded are T&&, so the object a co_yield expression designates is what *it refers to;
//     the yielded temporary lives until the coroutine resumes ([expr.await], full-expression).
//   /6-9: only an lvalue co_yield into an rvalue-reference yielded makes a copy (covered by
//     generator/reference_kinds).
//   /10-12: yield_value(elements_of(generator&&)) runs the nested generator; its yields reach
//     the consumer through the same mechanism, so they are not copied either; /13:
//     elements_of(r) for another range is a nested generator<yielded, void, Alloc> that yields
//     static_cast<yielded>(*i), so with a reference yielded the consumer sees r's elements.
// Pinned (support/inplace_probe.hpp) can be neither copied nor moved: a generator that compiles
// with it cannot have copied the yielded object.
#include <array>
#include <generator>
#include <ranges>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

const void* seen_local = nullptr;

std::generator<Pinned> pinned_values() {
  Arg a{5};
  co_yield Pinned(1, a);            // a prvalue: materialized, then referenced
  Pinned local(2, std::move(a));
  seen_local = &local;
  co_yield std::move(local);        // an xvalue: the consumer sees local itself
  co_yield {3, 7};                  // a braced-init-list: yielded is Pinned&&
}

std::generator<const Pinned&> const_refs() {
  Pinned local(4);
  seen_local = &local;
  co_yield local;                   // an lvalue binds to const Pinned&
  co_yield Pinned(5);               // so does a prvalue
}

std::generator<Pinned> inner() {
  Pinned local(6);
  seen_local = &local;
  co_yield std::move(local);
}

std::generator<Pinned> outer() {
  co_yield Pinned(0);
  co_yield std::ranges::elements_of(inner());
  co_yield Pinned(9);
}

std::generator<Pinned&> elements(std::array<Pinned, 2>& arr) { co_yield std::ranges::elements_of(arr); }

std::generator<Probe> probes() {
  co_yield Probe(1);
  Probe p(2);
  co_yield std::move(p);
}

int main() {
  probe::reset();
  {
    auto g = pinned_values();
    auto it = g.begin();
    static_assert(std::is_same_v<decltype(*it), Pinned&&>);
    CHECK((*it).key == 1 && (*it).cat == probe::lref);
    ++it;
    Pinned&& r = *it;
    CHECK(&r == seen_local && r.key == 2 && r.cat == probe::rref);
    ++it;
    CHECK((*it).key == 3 && (*it).extra == 7);
    ++it;
    CHECK(it == g.end());
    CHECK(counts.made == 3 && counts.extra() == 0 && counts.destroyed == 3);
  }

  probe::reset();
  {
    auto g = const_refs();
    auto it = g.begin();
    const Pinned& r = *it;
    CHECK(&r == seen_local && r.key == 4);
    ++it;
    CHECK((*it).key == 5);
    CHECK(counts.made == 2 && counts.extra() == 0);
  }

  probe::reset();
  {
    int keys[3] = {}, n = 0;
    bool inner_seen = false;
    for (Pinned&& p : outer()) {
      keys[n++] = p.key;
      if (p.key == 6) inner_seen = &p == seen_local;
    }
    CHECK(n == 3 && keys[0] == 0 && keys[1] == 6 && keys[2] == 9 && inner_seen);
    CHECK(counts.made == 3 && counts.extra() == 0 && counts.destroyed == 3);
  }

  {
    std::array<Pinned, 2> arr{{10, 11}};
    int n = 0;
    for (Pinned& p : elements(arr)) {
      CHECK(&p == &arr[static_cast<std::size_t>(n)]);
      ++n;
    }
    CHECK(n == 2);
  }

  // With a movable type nothing is copied or moved either (until the consumer does so).
  probe::reset();
  {
    int sum = 0;
    for (auto&& p : probes()) sum += p.key;
    CHECK(sum == 3 && counts.made == 2 && counts.extra() == 0);
  }
  return 0;
}
