// [map.access]/13-16 (C++26): lookup(x) returns optional<mapped_type&> (optional<const
// mapped_type&> on a const map) that refers to find(x)->second if contains(x), otherwise
// nullopt; it never inserts. The heterogeneous overloads take part only with a transparent
// comparator.
#include <map>
#include <functional>
#include <optional>
#include <type_traits>
#include "check.hpp"

constexpr bool lookup() {
  std::map<int, int> m{{1, 10}, {2, 20}};
  const auto& cm = m;
  static_assert(std::is_same_v<decltype(m.lookup(1)), std::optional<int&>>);
  static_assert(std::is_same_v<decltype(cm.lookup(1)), std::optional<const int&>>);
  auto r = m.lookup(2);
  if (!r || &*r != &m.at(2)) return false;
  *r = 21;
  if (m.at(2) != 21 || cm.lookup(3).has_value() || m.size() != 2) return false;
  if (cm.lookup(1).value() != 10) return false;
  std::map<long, int, std::less<>> t{{1, 10}};
  static_assert(std::is_same_v<decltype(t.lookup(1)), std::optional<int&>>);
  auto tr = t.lookup(1);  // int argument, no long temporary needed
  return tr && *tr == 10 && !t.lookup(2) && t.size() == 1;
}


template <class M>
concept has_lookup = requires(M& m) { m.lookup(1); };
static_assert(!has_lookup<std::multimap<int, int>>);

static_assert(lookup());

int main() {
  CHECK(lookup());
  return 0;
}
