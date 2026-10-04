// [ratio.arithmetic]/2: "If it is not possible to represent U or V with intmax_t, the program
// is ill-formed." INTMAX_MAX + 1 is not representable.
#include <ratio>
#include <cstdint>

using R = std::ratio_add<std::ratio<INTMAX_MAX, 1>, std::ratio<1, 1>>;
constexpr auto n = R::num;
