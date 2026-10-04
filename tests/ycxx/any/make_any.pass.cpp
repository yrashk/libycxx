// [any.nonmembers]/2-3: make_any<T>(args...) is equivalent to
// any(in_place_type<T>, std::forward<Args>(args)...), and the initializer_list overload to
// any(in_place_type<T>, il, std::forward<Args>(args)...).
#include <any>
#include <initializer_list>
#include <typeinfo>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
  P(int x, int y) : a(x), b(y) {}
};
struct L {
  int n, extra;
  L(std::initializer_list<int> il, int e) : n(static_cast<int>(il.size())), extra(e) {}
};

static_assert(std::is_same_v<decltype(std::make_any<int>(1)), std::any>);
static_assert(std::is_same_v<decltype(std::make_any<L>({1, 2}, 3)), std::any>);

int main() {
  std::any a = std::make_any<P>(1, 2);
  CHECK(a.type() == typeid(P));
  CHECK(std::any_cast<P&>(a).a == 1 && std::any_cast<P&>(a).b == 2);
  std::any b = std::make_any<L>({4, 5, 6}, 7);
  CHECK(std::any_cast<L&>(b).n == 3);
  CHECK(std::any_cast<L&>(b).extra == 7);
  std::any c = std::make_any<int>();
  CHECK(std::any_cast<int>(c) == 0);
  std::any d = std::make_any<const long>(2L);
  CHECK(d.type() == typeid(long));
  return 0;
}
