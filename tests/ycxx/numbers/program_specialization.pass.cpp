// [math.constants]/2: "a program may partially or explicitly specialize a mathematical
// constant variable template provided that the specialization depends on a program-defined
// type." The library's own specializations are constrained on floating_point, so a
// specialization for a program-defined type does not conflict with them.
#include <numbers>
#include "check.hpp"

struct Fixed {
  long long raw;  // value * 2^16
  friend constexpr bool operator==(Fixed, Fixed) = default;
};
template <> inline constexpr Fixed std::numbers::pi_v<Fixed>{205887};
template <> inline constexpr Fixed std::numbers::e_v<Fixed>{178145};

template <class T>
struct Wrap { T v; };
template <class T> inline constexpr Wrap<T> std::numbers::sqrt2_v<Wrap<T>>{std::numbers::sqrt2_v<T>};

static_assert(std::numbers::pi_v<Fixed>.raw == 205887);
static_assert(std::numbers::e_v<Fixed> == Fixed{178145});
static_assert(std::numbers::sqrt2_v<Wrap<float>>.v == std::numbers::sqrt2_v<float>);

int main() {
  CHECK(std::numbers::pi_v<Fixed>.raw == 205887);
  CHECK(std::numbers::sqrt2_v<Wrap<double>>.v == std::numbers::sqrt2);
  return 0;
}
