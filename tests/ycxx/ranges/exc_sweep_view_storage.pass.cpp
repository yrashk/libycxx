// Exception-injection sweep over range adaptors that store objects of user types: the
// movable-box holding a closure (transform_view, filter_view) or a value (single_view,
// repeat_view), and the non-propagating caches of join_view, join_with_view and
// cache_latest_view, which hold prvalue elements or inner ranges. The element type's
// constructors and assignments, and operator new (inner vectors), throw at their k-th call,
// for every k until the traversal completes.
//   [range.move.wrap]/1.3-1.4: movable-box copy/move assignment when T is not copyable/
//     movable (a lambda closure): "if (that) emplace(*that); else reset();": if the copy
//     throws, the box is left empty (its old object destroyed exactly once, nothing leaked;
//     the destructor afterwards destroys nothing more).
//   [range.nonprop.cache]: the cache's emplace-deref "destroys the stored object if any, then
//     direct-non-list-initializes": an exception leaves the cache empty.
//   [range.join.iterator], [range.join.with.iterator], [range.cache.latest]: the caches hold
//     the inner range / element of the current position.
// After every run every element object is destroyed exactly once and every operator new block
// is freed; traversal results are checked when nothing throws.
// REQUIRES: exceptions
#include <ranges>
#include <vector>
#include "exc_new.hpp"

using namespace exh;
namespace rv = std::views;

static const T base_t(100);
static const std::vector<T> pat_g{T(-1), T(-2)}; // created before any sweep

template <class F>
void sw(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

int main() {
  const auto K = {copy_ctor, move_ctor, value_ctor, copy_assign, move_assign};

  // movable-box of a closure that owns a T: copy/move assignment of the view.
  sw("transform_view = transform_view (closure owning a T)", K, [] {
    auto f = [t = base_t](int i) { return i + t.v; };
    auto v1 = rv::iota(0, 3) | rv::transform(f);
    auto v2 = v1;
    bool threw = attempt([&] {
      v1 = v2;
      v1 = std::move(v2);
    });
    return threw;
  });
  sw("filter_view = filter_view (closure owning a T)", K, [] {
    auto p = [t = base_t](int i) { return i < t.v; };
    auto v1 = rv::iota(0, 3) | rv::filter(p);
    auto v2 = v1;
    return attempt([&] { v1 = v2; });
  });
  sw("single_view<T> copy/assign", K, [] {
    std::ranges::single_view<T> a(base_t), b(T(5));
    return attempt([&] {
      a = b;
      std::ranges::single_view<T> c(a);
      a = std::move(c);
    });
  });
  sw("repeat_view<T> copy/assign and traversal", K, [] {
    auto a = rv::repeat(base_t, 3);
    auto b = rv::repeat(T(7), 2);
    return attempt([&] {
      a = b;
      int sum = 0;
      for (const T& x : a) sum += x.v;
      CHECK(sum == 14);
    });
  });

  // caches of prvalue elements / inner ranges
  sw("join over prvalue vector<T>", {copy_ctor, move_ctor, value_ctor, gnew}, [] {
    auto inner = [](int i) {
      std::vector<T> v;
      for (int j = 0; j <= i; ++j) v.emplace_back(10 * i + j);
      return v;
    };
    return attempt([&] {
      int n = 0, sum = 0;
      for (const T& x : rv::iota(0, 4) | rv::transform(inner) | rv::join) {
        ++n;
        sum += x.v;
      }
      CHECK(n == 10);
    });
  });
  sw("join_with over prvalue vector<T>, pattern vector<T>", {copy_ctor, move_ctor, value_ctor, gnew}, [] {
    auto inner = [](int i) { return std::vector<T>(std::size_t(i), T(i)); };
    return attempt([&] {
      int n = 0;
      for (const T& x : rv::iota(1, 4) | rv::transform(inner) | rv::join_with(pat_g)) {
        ++n;
        (void)x;
      }
      CHECK(n == 6 + 4);
    });
  });
  sw("cache_latest over prvalue T", {copy_ctor, move_ctor, value_ctor}, [] {
    return attempt([] {
      int sum = 0;
      auto v = rv::iota(0, 5) | rv::transform([](int i) { return T(i); }) | rv::cache_latest;
      for (auto it = v.begin(); it != v.end(); ++it) {
        sum += (*it).v;
        sum += (*it).v; // the second dereference uses the cache
      }
      CHECK(sum == 20);
    });
  });
  sw("cache_latest | filter over prvalue T", {copy_ctor, move_ctor, value_ctor}, [] {
    return attempt([] {
      int n = 0;
      auto v = rv::iota(0, 8) | rv::transform([](int i) { return T(i); }) | rv::cache_latest |
               rv::filter([](const T& t) { return t.v % 2 == 0; });
      for (const T& x : v) n += x.v >= 0;
      CHECK(n == 4);
    });
  });
  sw("join over cache_latest of prvalue vector<T>", {copy_ctor, move_ctor, value_ctor, gnew}, [] {
    return attempt([] {
      int n = 0;
      auto v = rv::iota(0, 3) | rv::transform([](int i) { return std::vector<T>(2, T(i)); }) | rv::cache_latest | rv::join;
      for (const T& x : v) n += x.v >= 0;
      CHECK(n == 6);
    });
  });
  (void)base_t;
  return finish();
}
