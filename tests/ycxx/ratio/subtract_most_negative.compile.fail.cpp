// [ratio.arithmetic]/1-2: ratio_subtract<R1, R2> denotes ratio<U, V> with U = ratio<X, Y>::num;
// here X = -INTMAX_MAX - 1 (representable in intmax_t, but) ratio<X, 1> is ill-formed by
// [ratio.ratio]/1 ("the absolute values of either of the template arguments ... not representable
// by type intmax_t", Note 1: "This excludes the most negative value"), so U cannot be formed: "If
// it is not possible to represent U or V with intmax_t, the program is ill-formed."
// Control (compiles): ratio_subtract<ratio<-INTMAX_MAX + 1>, ratio<1>>::num == -INTMAX_MAX.
#include <cstdint>
#include <ratio>

using R = std::ratio_subtract<std::ratio<-INTMAX_MAX, 1>, std::ratio<1, 1>>;
constexpr auto n = R::num;
