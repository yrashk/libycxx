// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::pair[^\n]*::pair\(
// EXPECT-ERROR-CLANG: error: (?:call to deleted constructor of [^\n]*std::pair|functional-style cast [^\n]*to [^\n]*std::pair[^\n]*uses deleted function)
// [pairs.pair]/13, /17: pair(U1&&, U2&&) ... "defined as deleted if ...
// reference_constructs_from_temporary_v<second_type, U2&&> is true."
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

static_assert(sizeof(std::pair<int, const long&>(1, 2)) > 0);
