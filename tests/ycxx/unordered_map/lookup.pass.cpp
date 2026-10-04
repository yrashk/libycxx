// [unord.map.elem] (C++26): lookup(k) returns optional<mapped_type&> (optional<const
// mapped_type&> on a const map) referring to the mapped value of the element with key k, or
// nullopt; it never inserts.
#include <unordered_map>
#include <optional>
#include <type_traits>
#include "check.hpp"

int main() {
  std::unordered_map<int, int> m{{1, 10}, {2, 20}};
  const auto& cm = m;
  static_assert(std::is_same_v<decltype(m.lookup(1)), std::optional<int&>>);
  static_assert(std::is_same_v<decltype(cm.lookup(1)), std::optional<const int&>>);
  auto r = m.lookup(2);
  CHECK(r && &*r == &m.at(2));
  *r = 21;
  CHECK(m.at(2) == 21 && !cm.lookup(3) && m.size() == 2 && cm.lookup(1).value() == 10);
  return 0;
}
