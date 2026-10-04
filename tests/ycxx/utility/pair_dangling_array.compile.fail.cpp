// [pairs.pair]/13, /17: the pair(P&&) constructor for pair-like P "is defined as deleted if ...
// reference_constructs_from_temporary_v<second_type, decltype(get<1>(FWD(p)))> is true."
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

static_assert(sizeof(std::pair<int, const int&>(asrc)) > 0);
