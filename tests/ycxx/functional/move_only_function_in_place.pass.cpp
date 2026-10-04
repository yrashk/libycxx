// [func.wrap.move.ctor]/10-21: explicit move_only_function(in_place_type_t<T>, Args&&...)
// and (in_place_type_t<T>, initializer_list<U>, Args&&...): "Constraints:
// is_constructible_v<VT, Args...> [resp. initializer_list<U>&, Args...] is true, and
// is-callable-from<VT> is true." "Postconditions: *this has a target object of type VT
// direct-non-list-initialized with std::forward<Args>(args)..." [resp. ilist, args...].
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Immovable {
  int a, b;
  Immovable(int x, int y) : a(x), b(y) {}
  Immovable(Immovable&&) = delete;
  int operator()() const { return a * 10 + b; }
};
struct FromList {
  int sum = 0;
  int extra;
  FromList(std::initializer_list<int> il, int e) : extra(e) {
    for (int x : il) sum += x;
  }
  int operator()() const { return sum + extra; }
};
struct Paren {  // direct-non-list-initialization: parentheses, not braces
  int which;
  Paren(int, int) : which(1) {}
  Paren(std::initializer_list<int>) : which(2) {}
  int operator()() const { return which; }
};
struct Aggregate {
  int x, y;
  int operator()() const { return x + y; }
};

using M = std::move_only_function<int() const>;
static_assert(std::is_constructible_v<M, std::in_place_type_t<Immovable>, int, int>);
static_assert(!std::is_constructible_v<M, std::in_place_type_t<Immovable>, int>);
static_assert(!std::is_convertible_v<std::in_place_type_t<Aggregate>, M>);  // explicit
static_assert(std::is_constructible_v<M, std::in_place_type_t<FromList>, std::initializer_list<int>, int>);
static_assert(!std::is_constructible_v<M, std::in_place_type_t<FromList>, std::initializer_list<long>, int>);
// the generic constructor is not used for in_place_type_t arguments
static_assert(!std::is_constructible_v<M, std::in_place_type_t<Immovable>&>);

int main() {
  M a(std::in_place_type<Immovable>, 4, 2);  // constructed in place: never moved
  CHECK(a() == 42);
  M moved = std::move(a);  // moving the wrapper does not need to move the target
  CHECK(moved() == 42);

  M b(std::in_place_type<FromList>, {1, 2, 3}, 4);
  CHECK(b() == 10);
  M c(std::in_place_type<Paren>, 1, 2);
  CHECK(c() == 1);
  M d(std::in_place_type<Aggregate>, 3, 4);  // parenthesized aggregate init
  CHECK(d() == 7);
  M e(std::in_place_type<int (*)()>, [] { return 5; });
  CHECK(e() == 5);
  M f(std::in_place_type<int (*)()>);  // value-initialized function pointer: target exists
  CHECK(static_cast<bool>(f));
  return 0;
}
