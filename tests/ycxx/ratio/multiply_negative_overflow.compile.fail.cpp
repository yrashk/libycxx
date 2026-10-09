// EXPECT-ERROR: error: static assertion failed[^\n]*\[ratio\.arithmetic\]/2: the result of the std::ratio arithmetic is not representable by intmax_t
// [ratio.arithmetic]/2: ratio_multiply<ratio<INTMAX_MAX>, ratio<-2>> is -2 INTMAX_MAX, whose
// U is not representable with intmax_t: the program is ill-formed.
// Control (compiles): ratio_multiply<ratio<INTMAX_MAX / 2>, ratio<-2>>::num == -(INTMAX_MAX - 1).
#include <cstdint>
#include <ratio>

using R = std::ratio_multiply<std::ratio<INTMAX_MAX, 1>, std::ratio<-2, 1>>;
constexpr auto n = R::num;
