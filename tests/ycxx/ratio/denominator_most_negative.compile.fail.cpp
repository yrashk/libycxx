// EXPECT-ERROR: error: static assertion failed[^\n]*\[ratio\.ratio\]/1: the absolute values of N and D must be representable by intmax_t
// [ratio.ratio]/1: "If the template argument D is zero or the absolute values of either of the
// template arguments N and D is not representable by type intmax_t, the program is ill-formed."
// |INTMAX_MIN| is not representable (most_negative covers N; this is D).
// Control (compiles): ratio<1, -INTMAX_MAX>::den == INTMAX_MAX, ::num == -1.
#include <cstdint>
#include <ratio>

constexpr auto d = std::ratio<1, INTMAX_MIN>::den;
