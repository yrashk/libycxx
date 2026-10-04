// [ratio.arithmetic]/2: "If it is not possible to represent U or V with intmax_t, the program
// is ill-formed." INTMAX_MAX * 2 is not representable.
#include <ratio>
#include <cstdint>

using R = std::ratio_multiply<std::ratio<INTMAX_MAX, 1>, std::ratio<2, 1>>;
constexpr auto n = R::num;
