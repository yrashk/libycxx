// [map.access]: operator[](const key_type&) / operator[](key_type&&) are
// try_emplace(x).first->second (insert a value-initialized mapped_type when the key is
// absent, otherwise return the existing one); at(x) returns the mapped value or throws
// out_of_range (const and non-const). The returned references stay valid as the map grows
// ([associative.reqmts.general]/175). multimap has none of these members.
// REQUIRES: exceptions
#include <map>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

struct MoveOnlyKey {
  int v;
  constexpr explicit MoveOnlyKey(int x) : v(x) {}
  constexpr MoveOnlyKey(MoveOnlyKey&& o) : v(o.v) { o.v = -1; }
  MoveOnlyKey(const MoveOnlyKey&) = delete;
  constexpr bool operator<(const MoveOnlyKey& o) const { return v < o.v; }
};

constexpr bool subscript() {
  std::map<int, Elem> m;
  static_assert(std::is_same_v<decltype(m[1]), Elem&>);
  Elem& a = m[3];  // value-initialized: Elem() holds -1
  if (m.size() != 1 || a.value() != -1) return false;
  a = Elem(30);
  for (int i = 0; i < 50; ++i) m[100 + i] = Elem(i);
  if (&m[3] != &a || m[3].value() != 30 || m.size() != 51) return false;
  int k = 7;
  m[k] = Elem(70);
  m[std::move(k)].heap[0] += 1;
  if (m.at(7).value() != 71 || m.size() != 52) return false;
  std::map<int, int> z;
  if (z[5] != 0) return false;  // int value-initialized to 0
  std::map<MoveOnlyKey, int> mk;
  mk[MoveOnlyKey(2)] = 20;  // operator[](key_type&&) moves the key in
  MoveOnlyKey key(2);
  mk[std::move(key)] = 21;  // existing: no effect on the key ("no effect" of try_emplace)
  return mk.size() == 1 && mk.begin()->second == 21 && key.v == 2;
}

bool at_throws() {
  std::map<int, int> m{{1, 10}};
  const auto& cm = m;
  static_assert(std::is_same_v<decltype(m.at(1)), int&> && std::is_same_v<decltype(cm.at(1)), const int&>);
  int caught = 0;
  try { (void)m.at(2); } catch (const std::out_of_range&) { ++caught; }
  try { (void)cm.at(0); } catch (const std::out_of_range&) { ++caught; }
  return caught == 2 && m.size() == 1 && m.at(1) == 10;
}

template <class X>
concept has_subscript = requires(X& x) { x[1]; };
template <class X>
concept has_at = requires(X& x) { x.at(1); };
static_assert(!has_subscript<std::multimap<int, int>> && !has_at<std::multimap<int, int>>);
static_assert(has_subscript<std::map<int, int>> && !has_subscript<const std::map<int, int>>);

static_assert(subscript());

int main() {
  CHECK(subscript());
  CHECK(at_throws());
  return 0;
}
