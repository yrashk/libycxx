// [pairs.pair]/13, /17: pair(U1&&, U2&&) "is defined as deleted if
// reference_constructs_from_temporary_v<first_type, U1&&> is true": a const S& member
// initialized from an int needs a temporary S.
// Checked in an unevaluated operand, so only the overload set matters (the mem-initializer
// rule in [class.base.init] for a temporary bound to a reference member never comes into play).
#include <utility>
#include <array>
#include <tuple>

struct S {
  S(int);
};
std::pair<long, int> src;
std::tuple<long, int> tsrc;
std::array<long, 2> asrc;

static_assert(sizeof(std::pair<const S&, int>(1, 2)) > 0);
