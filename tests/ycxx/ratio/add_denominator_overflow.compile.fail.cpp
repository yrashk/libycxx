// EXPECT-ERROR: error: static assertion failed[^\n]*\[ratio\.arithmetic\]/2: the result of the std::ratio arithmetic is not representable by intmax_t
// [ratio.arithmetic]/2: 1/INTMAX_MAX + 1/(INTMAX_MAX - 1) = (2 INTMAX_MAX - 1) /
// (INTMAX_MAX (INTMAX_MAX - 1)) in lowest terms (consecutive integers are coprime): V is not
// representable with intmax_t, so the program is ill-formed (a wrapped-around or truncated
// denominator must not be produced).
// Control (compiles): ratio_add<ratio<1, INTMAX_MAX>, ratio<1, INTMAX_MAX>> (2/INTMAX_MAX, or
// ill-formed by the X/Y rule; libycxx and libstdc++ give 2/INTMAX_MAX).
#include <cstdint>
#include <ratio>

using R = std::ratio_add<std::ratio<1, INTMAX_MAX>, std::ratio<1, INTMAX_MAX - 1>>;
constexpr auto d = R::den;
