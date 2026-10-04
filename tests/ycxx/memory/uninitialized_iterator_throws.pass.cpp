// [specialized.algorithms.general]/2: "Unless otherwise specified, if an exception is thrown in
// the following algorithms, objects constructed by a placement new-expression are destroyed in
// an unspecified order before allowing the exception to propagate." Here the exception comes
// from the SOURCE iterator's increment, after the element for the current position has been
// constructed ([uninitialized.copy]/2: "for (; first != last; ++result, (void)++first) ::new
// (voidify(*result)) ..."; [uninitialized.move] likewise; the _n forms "for (; n > 0;
// ++result, (void)++first, --n)"), so that element, and every earlier one, must be destroyed.
// Checked with an input iterator and a forward iterator, for the std and std::ranges forms
// ([specialized.algorithms]: the ranges forms have the same effects), at every throw point.
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include "check.hpp"

static int live = 0, budget = -1, failures = 0;
static void tick() {
  if (budget >= 0 && budget-- == 0) throw 5;
}
struct E {
  int v;
  E(int x) : v(x) { ++live; }
  E(const E& o) : v(o.v) { ++live; }
  E(E&& o) noexcept : v(o.v) { ++live; }
  ~E() { --live; }
};
static int src[6] = {1, 2, 3, 4, 5, 6};

template <class Tag>
struct It {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_category = Tag;
  using reference = int&;
  using pointer = int*;
  int* p = nullptr;
  int& operator*() const { return *p; }
  It& operator++() {
    tick();
    ++p;
    return *this;
  }
  It operator++(int) {
    It t = *this;
    ++*this;
    return t;
  }
  friend bool operator==(const It& a, const It& b) { return a.p == b.p; }
};
using In = It<std::input_iterator_tag>;
using Fwd = It<std::forward_iterator_tag>;
static_assert(std::input_iterator<In> && !std::forward_iterator<In> && std::forward_iterator<Fwd>);

template <class F>
void sweep(F f, int line) {
  alignas(E) unsigned char storage[sizeof(E) * 6];
  E* out = reinterpret_cast<E*>(storage);
  for (int limit = 0; limit < 6; ++limit) {
    budget = limit;
    bool threw = false;
    try {
      E* end = f(out);
      std::destroy(out, end);  // completed (not expected before the last limit)
    } catch (int e) {
      CHECK(e == 5);
      threw = true;
    }
    budget = -1;
    CHECK(threw);  // every limit < 6 is reached: 6 increments
    if (live != 0) {
      dprintf(2, "line %d: limit %d: %d objects not destroyed\n", line, limit, live);
      ++failures;
      live = 0;
      return;
    }
  }
}

template <class I>
void all(int line) {
  sweep([](E* o) { return std::uninitialized_copy(I{src}, I{src + 6}, o); }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::uninitialized_copy_n(I{src}, 6, o); }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::uninitialized_move(I{src}, I{src + 6}, o); }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::uninitialized_move_n(I{src}, 6, o).second; }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::ranges::uninitialized_copy(I{src}, I{src + 6}, o, o + 6).out; }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::ranges::uninitialized_copy_n(I{src}, 6, o, o + 6).out; }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::ranges::uninitialized_move(I{src}, I{src + 6}, o, o + 6).out; }, line * 1000 + __LINE__);
  sweep([](E* o) { return std::ranges::uninitialized_move_n(I{src}, 6, o, o + 6).out; }, line * 1000 + __LINE__);
}

int main() {
  all<In>(__LINE__);
  all<Fwd>(__LINE__);
  CHECK(failures == 0);
  return 0;
}
