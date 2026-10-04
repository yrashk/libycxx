// [hive.overview] synopsis: `constexpr hive() noexcept(noexcept(Allocator()))`,
// `constexpr explicit hive(const Allocator&) noexcept`, `constexpr explicit
// hive(hive_limits)` and `constexpr hive(hive_limits, const Allocator&)`; [hive.cons]/1-4:
// they construct an empty hive (the latter two initializing current-limits with the
// argument). A constexpr constructor whose call is a constant expression allows constant
// initialization ([basic.start.static]/2), which `constinit` ([dcl.constinit]/2) requires;
// the destructor need not be constexpr for a variable with static storage duration.
// [hive.capacity]/15: block_capacity_limits() returns current-limits.
#include <hive>
#include <cstddef>
#include <memory>
#include "check.hpp"

using H = std::hive<int>;
constexpr std::hive_limits def = H::block_capacity_default_limits();

constinit H g1;
constinit H g2{std::allocator<int>()};
constinit H g3{def};
constinit H g4{def, std::allocator<int>()};

int main() {
  CHECK(g1.empty() && g2.empty() && g3.empty() && g4.empty());
  CHECK(g3.block_capacity_limits().min == def.min && g3.block_capacity_limits().max == def.max);
  CHECK(g4.block_capacity_limits().min == def.min && g4.block_capacity_limits().max == def.max);
  for (int i = 0; i < 100; ++i) g1.insert(i);
  CHECK(g1.size() == 100);
  return 0;
}
