// [container.reqmts]/48-51: t.swap(s) has type void and exchanges the contents of t and s;
// swap(t, s) is equivalent to t.swap(s). /65: for standard containers other than array and
// inplace_vector, a.swap(b) exchanges the values "without invoking any move, copy, or swap
// operations on the individual container elements", and "every iterator referring to an
// element in one container before the swap shall refer to the same element in the other
// container after the swap". (basic_string is not covered by the iterator part:
// [string.require]/4 lets swap invalidate its iterators.)
#include <vector>
#include <string>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr bool contents() {
  static_assert(std::is_same_v<decltype(std::declval<X&>().swap(std::declval<X&>())), void>);
  X a = make<X>({1, 2, 3});
  X b = make<X>({4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  a.swap(b);
  if (!holds(b, {1, 2, 3}) || !holds(a, {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}))
    return false;
  using std::swap;
  swap(a, b);
  if (!holds(a, {1, 2, 3}) || b.size() != 21) return false;
  X e;
  a.swap(e);
  if (!a.empty() || !holds(e, {1, 2, 3})) return false;
  swap(e, a);
  if (!e.empty() || !holds(a, {1, 2, 3})) return false;
  a.swap(a);
  if (!holds(a, {1, 2, 3})) return false;
  return true;
}

template <class X>
constexpr bool iterators_follow() {
  X a = make<X>({1, 2, 3});
  X b = make<X>({4, 5});
  auto ia = a.begin() + 1;
  auto pa = std::addressof(*ia);
  auto ib = b.begin();
  auto pb = std::addressof(*ib);
  a.swap(b);
  // ia now refers to an element of b, ib to an element of a
  if (std::addressof(*ia) != pa || std::addressof(*ib) != pb) return false;
  if (!(*ia == val<typename X::value_type>(2)) || !(*ib == val<typename X::value_type>(4))) return false;
  if (std::addressof(*(b.begin() + 1)) != pa || std::addressof(*a.begin()) != pb) return false;
  ++ia;
  ++ia;
  if (ia != b.end()) return false;
  return true;
}

struct CountOps {
  static inline int copies = 0, moves = 0, swaps = 0;
  int v;
  CountOps(int x) : v(x) {}
  CountOps(const CountOps& o) : v(o.v) { ++copies; }
  CountOps(CountOps&& o) noexcept : v(o.v) { ++moves; }
  CountOps& operator=(const CountOps& o) { v = o.v; ++copies; return *this; }
  CountOps& operator=(CountOps&& o) noexcept { v = o.v; ++moves; return *this; }
  friend void swap(CountOps& a, CountOps& b) noexcept { std::swap(a.v, b.v); ++swaps; }
};

static_assert(contents<std::vector<int>>());
static_assert(contents<std::vector<Elem>>());
static_assert(contents<std::vector<bool>>());
static_assert(contents<std::string>());
static_assert(iterators_follow<std::vector<int>>());
static_assert(iterators_follow<std::vector<Elem>>());

int main() {
  CHECK(contents<std::vector<int>>());
  CHECK(contents<std::vector<Elem>>());
  CHECK(contents<std::vector<bool>>());
  CHECK(contents<std::vector<std::string>>());
  CHECK(contents<std::string>());
  CHECK(contents<std::wstring>());
  CHECK(iterators_follow<std::vector<int>>());
  CHECK(iterators_follow<std::vector<Elem>>());

  std::vector<CountOps> a, b;
  a.reserve(4);
  b.reserve(4);
  a.emplace_back(1);
  a.emplace_back(2);
  b.emplace_back(3);
  CountOps::copies = CountOps::moves = CountOps::swaps = 0;
  a.swap(b);
  using std::swap;
  swap(a, b);
  CHECK(CountOps::copies == 0 && CountOps::moves == 0 && CountOps::swaps == 0);
  CHECK(a.size() == 2 && a[0].v == 1 && b.size() == 1 && b[0].v == 3);
  return 0;
}
