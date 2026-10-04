// [ratio.arithmetic]/2, Table 65: ratio_divide<R1, R2> is X/Y with X = R1::num * R2::den,
// Y = R1::den * R2::num; INTMAX_MAX / (1/2) = 2 * INTMAX_MAX is not representable: "If it is
// not possible to represent U or V with intmax_t, the program is ill-formed."
// Control (compiles): ratio_divide<ratio<INTMAX_MAX / 2>, ratio<1, 2>>::num == INTMAX_MAX - 1.
#include <cstdint>
#include <ratio>

using R = std::ratio_divide<std::ratio<INTMAX_MAX, 1>, std::ratio<1, 2>>;
constexpr auto n = R::num;
