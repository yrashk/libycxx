// [polymorphic.syn]: all members of polymorphic are constexpr; with constexpr virtual
// functions and destructors it works during constant evaluation.
#include <memory>
#include <utility>

struct Base {
  constexpr virtual ~Base() = default;
  constexpr virtual int value() const { return 1; }
};
struct Derived : Base {
  int x;
  constexpr explicit Derived(int v) : x(v) {}
  constexpr int value() const override { return x; }
};

constexpr bool run() {
  std::polymorphic<Base> a;
  std::polymorphic<Base> b(std::in_place_type<Derived>, 42);
  if (a->value() != 1 || b->value() != 42) return false;
  std::polymorphic<Base> c = b;
  if (c->value() != 42) return false;
  a = c;
  if (a->value() != 42) return false;
  std::polymorphic<Base> d(std::move(a));
  d.swap(b);
  return d->value() == 42 && b->value() == 42;
}
static_assert(run());

int main() { return run() ? 0 : 1; }
