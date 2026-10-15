// The extension's value semantics, ownership, forwarding and relocation guarantees.
#include <ycxx/functional.hpp>
#include <memory>
#include <utility>
#include "check.hpp"

using W = ycxx::inplace_function<int(int)>;
struct Counter {
  int n = 0;
  int operator()(int x) { return x + ++n; }
};
struct Tracked {
  static inline int alive = 0;
  const Tracked* self = this;
  Tracked() { ++alive; }
  Tracked(const Tracked&) : Tracked() {}
  Tracked(Tracked&&) noexcept : Tracked() {}
  ~Tracked() { CHECK(self == this); --alive; }
  int operator()(int x) { CHECK(self == this); return x; }
};
int twice(int x) { return x * 2; }
struct Object {
  int n = 3;
  int get() { return n; }
};

int main() {
  const W empty;
  CHECK(!empty && empty == nullptr && nullptr == empty);
  ycxx::inplace_function<int(), 1, 1> tiny = [] { return 1; };
  CHECK(tiny() == 1);
  W f = Counter{};
  const W& cf = f;
  CHECK(cf(1) == 2);
  W g(f);
  CHECK(g(1) == 3 && f(1) == 3); // independent target copies
  W h(std::move(g));
  CHECK(!g && h(0) == 3);
  g = twice;
  swap(g, h);
  CHECK(h(3) == 6 && g(0) == 4);
  g = g;
  CHECK(g(0) == 5);
  auto* same = &g;
  g = std::move(*same);
  CHECK(g(0) == 6);
  W vacant;
  vacant.swap(g);
  CHECK(!g && vacant(0) == 7);
  vacant = nullptr;
  CHECK(!vacant);
  g = static_cast<int (*)(int)>(nullptr);
  CHECK(!g);
  ycxx::inplace_function<int(Object&)> member = &Object::get;
  Object object;
  CHECK(member(object) == 3);
  member = static_cast<int (Object::*)()>(nullptr);
  CHECK(!member);
  Counter counter;
  W ref = std::ref(counter);
  CHECK(ref(1) == 2 && counter.n == 1);

  {
    W a = Tracked{};
    W b = a;
    W c = std::move(a);
    CHECK(Tracked::alive == 2 && !a && b(5) == 5 && c(6) == 6);
    a = std::move(b);
    a.swap(c);
    a.swap(a);
    CHECK(Tracked::alive == 2 && a(7) == 7 && c(8) == 8);
  }
  CHECK(Tracked::alive == 0);

  int n = 3;
  ycxx::inplace_function<int&()> reference = [&]() -> int& { return n; };
  reference() = 4;
  CHECK(n == 4);
  ycxx::inplace_function<void(std::unique_ptr<int>)> consuming = [&](std::unique_ptr<int> p) { n = *p; return 99; };
  consuming(std::make_unique<int>(8));
  CHECK(n == 8);

  // Assignment must take the source before deleting the target that owns it.
  auto owner = std::make_shared<W>(twice);
  W* nested = owner.get();
  W outer = [owner](int x) { return (*owner)(x); };
  owner.reset();
  outer = std::move(*nested);
  CHECK(outer(5) == 10);
}
