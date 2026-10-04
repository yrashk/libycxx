// [flat.map.access] (C++26): lookup(x) returns optional<mapped_type&> (optional<const
// mapped_type&> for a const flat_map) referring to find(x)->second if contains(x),
// otherwise nullopt; it never inserts.
#include <flat_map>
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

constexpr bool lookup() {
  std::flat_map<int, int> m{{1, 10}};
  static_assert(std::is_same_v<decltype(m.lookup(1)), std::optional<int&>>);
  static_assert(std::is_same_v<decltype(std::as_const(m).lookup(1)), std::optional<const int&>>);
  auto r = m.lookup(1);
  if (!r) return false;
  *r = 11;
  return m.at(1) == 11 && !m.lookup(2) && m.size() == 1;
}

static_assert(lookup());

int main() {
  CHECK(lookup());
  return 0;
}
