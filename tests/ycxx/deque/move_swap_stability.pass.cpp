// [container.reqmts]/15-16, /50, /65 and [container.alloc.reqmts]/17,20 for deque: move
// construction (also with an equal allocator), move assignment with std::allocator and swap
// do not copy or move individual elements; the elements stay where they are and are owned
// by the destination, and after swap iterators refer to the same elements in the other
// container.
#include <deque>
#include "container_values.hpp"
#include "reqs/move_swap_stability.hpp"
#include "check.hpp"

template <class T>
constexpr std::deque<T> make_n(int n) {
  std::deque<T> d;
  for (int i = 0; i < n; ++i) {
    if (i % 2) d.push_back(val<T>(i));
    else d.push_front(val<T>(i));
  }
  return d;
}

struct Counted {
  static inline int copies = 0, moves = 0;
  int v;
  Counted(int x) : v(x) {}
  Counted(const Counted& o) : v(o.v) { ++copies; }
  Counted(Counted&& o) noexcept : v(o.v) { ++moves; }
  Counted& operator=(const Counted& o) { v = o.v; ++copies; return *this; }
  Counted& operator=(Counted&& o) noexcept { v = o.v; ++moves; return *this; }
};

static_assert(reqs::move_swap_stability::test<std::deque<int>>(make_n<int>));
static_assert(reqs::move_swap_stability::test<std::deque<Elem>>(make_n<Elem>));

int main() {
  CHECK(reqs::move_swap_stability::test<std::deque<int>>(make_n<int>));
  CHECK(reqs::move_swap_stability::test<std::deque<Elem>>(make_n<Elem>));

  std::deque<Counted> a;
  for (int i = 0; i < 100; ++i) a.emplace_back(i);
  Counted::copies = Counted::moves = 0;
  std::deque<Counted> b(std::move(a));
  std::deque<Counted> c(std::move(b), std::allocator<Counted>());
  std::deque<Counted> d;
  d = std::move(c);
  CHECK(Counted::copies == 0 && Counted::moves == 0);
  std::deque<Counted> e{Counted(1)};
  Counted::copies = Counted::moves = 0;
  d.swap(e);
  CHECK(Counted::copies == 0 && Counted::moves == 0);
  CHECK(e.size() == 100 && e[99].v == 99 && d.size() == 1);
  return 0;
}
