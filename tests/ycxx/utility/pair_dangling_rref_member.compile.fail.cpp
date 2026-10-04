// [pairs.pair]/13, /17: the pair(pair<U1, U2>&&) constructor is deleted when
// "reference_constructs_from_temporary_v<first_type, decltype(get<0>(FWD(p)))>" is true: an
// int&& member from a long&& would bind to a temporary int.
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

static_assert(sizeof(std::pair<int&&, int>(std::move(src))) > 0);
