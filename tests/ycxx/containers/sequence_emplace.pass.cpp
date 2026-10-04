// [sequence.reqmts]/20-23: a.emplace(p, args) has type iterator, inserts an object of type T
// constructed with std::forward<Args>(args)... before p, and returns an iterator to the new
// element; Note 1: "args can directly or indirectly refer to a value in a". /84-87:
// a.emplace_back(args) has type reference, appends T(std::forward<Args>(args)...) and returns
// a.back(). Multiple constructor arguments are forwarded unchanged (rvalues stay rvalues).
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

struct Two {
  int a;
  int b;
  bool from_rvalue = false;
  constexpr Two(int x, int y) : a(x), b(y) {}
  constexpr Two(const Elem& e, int y) : a(e.value()), b(y) {}
  constexpr Two(Elem&& e, int y) : a(e.value()), b(y), from_rvalue(true) {}
  constexpr bool operator==(const Two& o) const { return a == o.a && b == o.b; }
};

template <class X>
constexpr bool generic() {
  using T = typename X::value_type;
  using It = typename X::iterator;
  static_assert(std::is_same_v<decltype(std::declval<X&>().emplace(std::declval<X&>().cbegin(), val<T>(1))), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().emplace_back(val<T>(1))), typename X::reference>);
  X a = make<X>({1, 2, 3});
  It r = a.emplace(a.cbegin() + 1, val<T>(5));
  if (r != a.begin() + 1 || !holds(a, {1, 5, 2, 3})) return false;
  r = a.emplace(a.cend(), val<T>(6));
  if (r != a.end() - 1 || !holds(a, {1, 5, 2, 3, 6})) return false;
  r = a.emplace(a.cbegin(), val<T>(7));
  if (r != a.begin() || !holds(a, {7, 1, 5, 2, 3, 6})) return false;
  // argument referring to an element of a, with and without reallocation
  for (int k = 0; k < 40; ++k) a.emplace(a.cbegin() + 1, a.back());
  if (count_elems(a) != 46 || !(a[1] == val<T>(6)) || !(a[40] == val<T>(6)) || !(a[41] == val<T>(1))) return false;
  a.emplace(a.cbegin(), a[2]);
  if (!(a[0] == val<T>(6))) return false;

  X b;
  auto&& ref = b.emplace_back(val<T>(4));
  if (!(ref == val<T>(4))) return false;
  for (int k = 0; k < 40; ++k) {
    auto&& r2 = b.emplace_back(b.front());  // refers into b, reallocation along the way
    if (!(r2 == val<T>(4))) return false;
  }
  if (count_elems(b) != 41) return false;
  return true;
}

constexpr bool multi_arg() {
  std::vector<Two> v;
  Two& r = v.emplace_back(1, 2);
  if (&r != &v.back() || r.a != 1 || r.b != 2) return false;
  auto it = v.emplace(v.cbegin(), 3, 4);
  if (it != v.begin() || v[0].a != 3 || v.size() != 2) return false;
  Elem e(9);
  v.emplace_back(e, 1);
  if (v.back().from_rvalue || e.value() != 9) return false;
  v.emplace_back(std::move(e), 1);
  if (!v.back().from_rvalue) return false;
  v.emplace(v.cbegin() + 1, Elem(5), 0);
  if (!v[1].from_rvalue || v[1].a != 5) return false;
  if (std::addressof(v.emplace_back(0, 0)) != std::addressof(v.back())) return false;
  return true;
}

static_assert(generic<std::vector<int>>());
static_assert(generic<std::vector<Elem>>());
static_assert(generic<std::vector<bool>>());
static_assert(multi_arg());

int main() {
  CHECK(generic<std::vector<int>>());
  CHECK(generic<std::vector<Elem>>());
  CHECK(generic<std::vector<bool>>());
  CHECK(generic<std::vector<std::string>>());
  CHECK(multi_arg());
  return 0;
}
