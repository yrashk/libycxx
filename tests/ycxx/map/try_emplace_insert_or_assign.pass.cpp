// [map.modifiers]/3-31: try_emplace(k, args...) has no effect when the key exists (in
// particular the arguments are not moved from) and otherwise inserts value_type constructed
// piecewise from (k) and (args...); the result's bool is true iff inserted and the iterator
// points to the element with key k. insert_or_assign(k, obj) assigns std::forward<M>(obj) to
// the existing element's mapped value or inserts value_type(k, obj); same result convention.
// The hinted forms return just the iterator. Key arguments that are rvalues are moved from
// only when an insertion happens.
#include <map>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Res {
  int v = 0;
  bool moved_from = false;
  int ctor_args = 0;
  constexpr Res() = default;
  constexpr Res(int a) : v(a), ctor_args(1) {}
  constexpr Res(int a, int b) : v(a * 100 + b), ctor_args(2) {}
  constexpr Res(Res&& o) : v(o.v), ctor_args(o.ctor_args) { o.moved_from = true; }
  constexpr Res(const Res& o) = default;
  constexpr Res& operator=(Res&& o) {
    v = o.v;
    o.moved_from = true;
    return *this;
  }
  constexpr Res& operator=(const Res&) = default;
};

struct Key {
  int k;
  bool moved_from = false;
  constexpr Key(int x) : k(x) {}
  constexpr Key(Key&& o) : k(o.k) { o.moved_from = true; }
  constexpr Key(const Key&) = default;
  constexpr bool operator<(const Key& o) const { return k < o.k; }
};

constexpr bool try_emplace() {
  std::map<Key, Res> m;
  using It = std::map<Key, Res>::iterator;
  static_assert(std::is_same_v<decltype(m.try_emplace(Key(1))), std::pair<It, bool>>);
  static_assert(std::is_same_v<decltype(m.try_emplace(m.cend(), Key(1))), It>);
  auto [i1, b1] = m.try_emplace(Key(1), 2, 3);
  if (!b1 || i1->first.k != 1 || i1->second.v != 203 || i1->second.ctor_args != 2) return false;
  Res r(9);
  Key k(1);
  auto [i2, b2] = m.try_emplace(std::move(k), std::move(r));
  if (b2 || i2 != i1 || r.moved_from || k.moved_from || i1->second.v != 203) return false;
  Key k2(2);
  auto [i3, b3] = m.try_emplace(std::move(k2), std::move(r));
  if (!b3 || !r.moved_from || !k2.moved_from || i3->second.v != 9) return false;
  const Key k3(3);
  auto [i4, b4] = m.try_emplace(k3);  // default-constructed mapped value
  if (!b4 || i4->second.v != 0 || i4->second.ctor_args != 0) return false;
  Res r2(5);
  auto h = m.try_emplace(m.cend(), Key(3), std::move(r2));
  if (h != i4 || r2.moved_from || m.size() != 3) return false;
  h = m.try_emplace(m.cbegin(), Key(0), 7);
  if (h != m.begin() || h->second.v != 7 || m.size() != 4) return false;
  return true;
}

constexpr bool insert_or_assign() {
  std::map<Key, Res> m;
  using It = std::map<Key, Res>::iterator;
  static_assert(std::is_same_v<decltype(m.insert_or_assign(Key(1), Res())), std::pair<It, bool>>);
  static_assert(std::is_same_v<decltype(m.insert_or_assign(m.cend(), Key(1), Res())), It>);
  auto [i1, b1] = m.insert_or_assign(Key(1), Res(10));
  if (!b1 || i1->second.v != 10) return false;
  const Res* addr = std::addressof(i1->second);
  Res r(11);
  Key k(1);
  auto [i2, b2] = m.insert_or_assign(std::move(k), std::move(r));
  if (b2 || i2 != i1 || i1->second.v != 11 || !r.moved_from || k.moved_from) return false;
  if (std::addressof(i1->second) != addr) return false;  // assigned in place
  const Res cr(12);
  auto [i3, b3] = m.insert_or_assign(Key(1), cr);
  if (b3 || i3->second.v != 12) return false;
  auto h = m.insert_or_assign(m.cend(), Key(2), Res(20));
  if (h->first.k != 2 || h->second.v != 20 || m.size() != 2) return false;
  h = m.insert_or_assign(m.cbegin(), Key(2), 21);  // M = int, assigned through Res(int)
  if (h->second.v != 21 || m.size() != 2) return false;
  return true;
}

static_assert(try_emplace());
static_assert(insert_or_assign());

int main() {
  CHECK(try_emplace());
  CHECK(insert_or_assign());
  // a move-only mapped type works with try_emplace
  std::map<int, std::unique_ptr<int>> u;
  auto p = std::make_unique<int>(5);
  u.try_emplace(1, std::move(p));
  auto q = std::make_unique<int>(6);
  auto [it, ins] = u.try_emplace(1, std::move(q));
  CHECK(!ins && q && *q == 6 && *it->second == 5);
  u.insert_or_assign(1, std::move(q));
  CHECK(!q && *u[1] == 6);
  return 0;
}
