// [unique.ptr.create]: make_unique<T>(args...) returns unique_ptr<T>(new T(std::forward<Args>
// (args)...)) — parenthesized, so aggregates work too; make_unique<T[]>(n) returns
// unique_ptr<T>(new remove_extent_t<T>[n]()) — value-initialized elements;
// make_unique_for_overwrite<T>() returns unique_ptr<T>(new T) (default-initialized) and
// make_unique_for_overwrite<T[]>(n) unique_ptr<T>(new remove_extent_t<T>[n]). The
// known-bound forms are deleted; all forms are constexpr.
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Agg {
  int a;
  int b;
};
struct Ctor {
  int v;
  std::string s;
  Ctor(int x, std::string y) : v(x), s(std::move(y)) {}
};
struct DefaultInit {
  int marker = 42;
  int unset;  // left indeterminate by default-initialization
};
struct MoveOnlyArg {
  int v;
  explicit MoveOnlyArg(std::unique_ptr<int> p) : v(*p) {}
};

constexpr bool test() {
  auto a = std::make_unique<int>();
  if (*a != 0) return false;  // new int() value-initializes
  auto b = std::make_unique<int>(5);
  if (*b != 5) return false;
  auto c = std::make_unique<Agg>(1, 2);
  if (c->a != 1 || c->b != 2) return false;
  auto d = std::make_unique<int[]>(4);
  for (int i = 0; i < 4; ++i)
    if (d[i] != 0) return false;
  auto e = std::make_unique_for_overwrite<int[]>(3);
  for (int i = 0; i < 3; ++i) e[i] = i;  // only writing is allowed before the first write
  if (e[2] != 2) return false;
  auto f = std::make_unique_for_overwrite<DefaultInit>();
  if (f->marker != 42) return false;
  auto g = std::make_unique<int[]>(0);
  if (!g) return false;  // new int[0]() returns a non-null pointer
  return true;
}

static_assert(std::is_same_v<decltype(std::make_unique<int>()), std::unique_ptr<int>>);
static_assert(std::is_same_v<decltype(std::make_unique<int[]>(1)), std::unique_ptr<int[]>>);
static_assert(std::is_same_v<decltype(std::make_unique<const int>(1)), std::unique_ptr<const int>>);
static_assert(std::is_same_v<decltype(std::make_unique_for_overwrite<int>()), std::unique_ptr<int>>);
static_assert(std::is_same_v<decltype(std::make_unique_for_overwrite<int[]>(1)), std::unique_ptr<int[]>>);

template <class T, class... A>
concept can_make = requires(A... a) { std::make_unique<T>(a...); };
template <class T, class... A>
concept can_make_ow = requires(A... a) { std::make_unique_for_overwrite<T>(a...); };
static_assert(can_make<int[], std::size_t>);
static_assert(!can_make<int[3]>);
static_assert(!can_make<int[3], std::size_t>);
static_assert(!can_make_ow<int[3]>);
static_assert(!can_make_ow<int[3], std::size_t>);
static_assert(!can_make_ow<int, int>);
static_assert(!can_make_ow<int[]>);

struct Counted {
  static inline int ctor = 0;
  Counted() { ++ctor; }
};

int main() {
  CHECK(test());
  static_assert(test());
  auto c = std::make_unique<Ctor>(3, "three");
  CHECK(c->v == 3 && c->s == "three");
  auto m = std::make_unique<MoveOnlyArg>(std::make_unique<int>(9));  // perfect forwarding
  CHECK(m->v == 9);
  auto arr = std::make_unique<Counted[]>(5);
  CHECK(Counted::ctor == 5);
  auto ow = std::make_unique_for_overwrite<Counted[]>(2);  // default-init still runs ctors
  CHECK(Counted::ctor == 7);
  auto s = std::make_unique<std::string[]>(2);
  CHECK(s[0].empty() && s[1].empty());
  return 0;
}
