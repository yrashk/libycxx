// [func.wrap.copy.ctor]/12-23: explicit copyable_function(in_place_type_t<T>, Args&&...) and
// (in_place_type_t<T>, initializer_list<U>, Args&&...): constrained on is_constructible_v<VT,
// Args...> [resp. initializer_list<U>&, Args...] and is-callable-from<VT>; "Postconditions:
// *this has a target object of type VT direct-non-list-initialized with
// std::forward<Args>(args)..." [resp. ilist, std::forward<Args>(args)...].
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Pair {
  int a, b;
  Pair(int x, int y) : a(x), b(y) {}
  int operator()() const { return a * 10 + b; }
};
struct FromList {
  int sum = 0;
  FromList(std::initializer_list<int> il, int e) : sum(e) {
    for (int x : il) sum += x;
  }
  int operator()() const { return sum; }
};
struct Paren {
  int which;
  Paren(int, int) : which(1) {}
  Paren(std::initializer_list<int>) : which(2) {}
  int operator()() const { return which; }
};

using C = std::copyable_function<int() const>;
static_assert(std::is_constructible_v<C, std::in_place_type_t<Pair>, int, int>);
static_assert(!std::is_constructible_v<C, std::in_place_type_t<Pair>, int>);
static_assert(!std::is_convertible_v<std::in_place_type_t<Pair>, C>);
static_assert(std::is_constructible_v<C, std::in_place_type_t<FromList>, std::initializer_list<int>, int>);
static_assert(!std::is_constructible_v<C, std::in_place_type_t<FromList>, std::initializer_list<int>>);

int main() {
  C a(std::in_place_type<Pair>, 4, 2);
  CHECK(a() == 42);
  C copy = a;
  CHECK(copy() == 42);
  C b(std::in_place_type<FromList>, {1, 2, 3}, 4);
  CHECK(b() == 10);
  C c(std::in_place_type<Paren>, 1, 2);
  CHECK(c() == 1);
  C d(std::in_place_type<int (*)()>);  // a value-initialized pointer is still a target object
  CHECK(static_cast<bool>(d));
  return 0;
}
