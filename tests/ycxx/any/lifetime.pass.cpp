// [any.cons], [any.assign], [any.modifiers], [any.class.general]: an any owns its contained
// value: copying copies it, moving leaves the source in a valid state, and every contained
// object constructed (by the value, in_place, copy, emplace or assignment operations) is
// destroyed exactly once (by reset, assignment, emplace, or ~any), for small and large types.
#include <any>
#include <utility>
#include "check.hpp"

static int live = 0, made = 0;
template <int N>
struct Obj {
  char pad[N];
  int v;
  explicit Obj(int x) : pad{}, v(x) { ++live, ++made; }
  Obj(const Obj& o) : pad{}, v(o.v) { ++live, ++made; }
  Obj(Obj&& o) noexcept : pad{}, v(o.v) { ++live, ++made; }
  ~Obj() { --live; }
};

template <int N>
void run() {
  live = made = 0;
  {
    std::any a(Obj<N>(1));
    CHECK(live == 1);
    std::any b = a;
    CHECK(live == 2 && std::any_cast<Obj<N>&>(b).v == 1);
    std::any c = std::move(a);
    CHECK(live >= 2 && live <= 3);  // a may or may not still hold a (moved-from) value
    a.reset();
    CHECK(live == 2);
    c.emplace<Obj<N>>(5);
    CHECK(live == 2 && std::any_cast<Obj<N>&>(c).v == 5);
    b = c;
    CHECK(live == 2 && std::any_cast<Obj<N>&>(b).v == 5);
    b = 42;  // replaces the contained Obj
    CHECK(live == 1);
    b.swap(c);
    CHECK(live == 1 && std::any_cast<Obj<N>&>(b).v == 5 && std::any_cast<int>(c) == 42);
    std::any d(std::in_place_type<Obj<N>>, 7);
    CHECK(live == 2);
    d = std::move(b);
    CHECK(live >= 1 && live <= 2 && std::any_cast<Obj<N>&>(d).v == 5);
    b.reset();
    CHECK(live == 1);
  }
  CHECK(live == 0 && made > 0);
}

int main() {
  run<1>();    // small: likely stored inline
  run<256>();  // large: stored on the heap
  return 0;
}
