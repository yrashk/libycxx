// [ratio.arithmetic]: ratio_add/subtract/multiply/divide denote ratio<U, V> with U and V the
// reduced num and den of ratio<X, Y> (Table 65), including the draft's Example 1 (which
// "may cause the program to be ill-formed under some implementations" only for the INT_MAX
// cases, whose results fit in intmax_t here). [ratio.comparison]: ratio_equal compares num and
// den; ratio_less compares R1::num * R2::den < R2::num * R1::den, avoiding overflow where
// possible (or the program is ill-formed); the rest are defined from them; the _v variable templates match.
#include <ratio>
#include <climits>
#include <cstdint>
#include <type_traits>

using std::ratio;
static_assert(std::ratio_add<ratio<1, 3>, ratio<1, 6>>::num == 1);
static_assert(std::ratio_add<ratio<1, 3>, ratio<1, 6>>::den == 2);
static_assert(std::ratio_multiply<ratio<1, 3>, ratio<3, 2>>::num == 1);
static_assert(std::ratio_multiply<ratio<1, 3>, ratio<3, 2>>::den == 2);
static_assert(std::ratio_add<ratio<1, INT_MAX>, ratio<1, INT_MAX>>::num == 2);
static_assert(std::ratio_add<ratio<1, INT_MAX>, ratio<1, INT_MAX>>::den == INT_MAX);
static_assert(std::ratio_multiply<ratio<1, INT_MAX>, ratio<INT_MAX, 2>>::num == 1);
static_assert(std::ratio_multiply<ratio<1, INT_MAX>, ratio<INT_MAX, 2>>::den == 2);

// The aliases denote ratio specializations directly (not types derived from them).
static_assert(std::is_same_v<std::ratio_add<ratio<1, 2>, ratio<1, 3>>, ratio<5, 6>>);
static_assert(std::is_same_v<std::ratio_subtract<ratio<1, 2>, ratio<1, 3>>, ratio<1, 6>>);
static_assert(std::is_same_v<std::ratio_subtract<ratio<1, 3>, ratio<1, 2>>, ratio<-1, 6>>);
static_assert(std::is_same_v<std::ratio_multiply<ratio<-2, 3>, ratio<9, 4>>, ratio<-3, 2>>);
static_assert(std::is_same_v<std::ratio_divide<ratio<2, 3>, ratio<-4, 9>>, ratio<-3, 2>>);
static_assert(std::is_same_v<std::ratio_add<ratio<6, 4>, ratio<0>>, ratio<3, 2>>);
static_assert(std::is_same_v<std::ratio_subtract<ratio<1, 2>, ratio<2, 4>>, ratio<0, 1>>);

static_assert(std::ratio_equal_v<ratio<1, 2>, ratio<2, 4>>);
static_assert(std::ratio_equal<ratio<1, 2>, ratio<2, 4>>::value);
static_assert(std::is_base_of_v<std::true_type, std::ratio_equal<ratio<1, 2>, ratio<3, 6>>>);
static_assert(std::is_base_of_v<std::false_type, std::ratio_equal<ratio<1, 2>, ratio<1, 3>>>);
static_assert(std::ratio_not_equal_v<ratio<1, 2>, ratio<-1, 2>>);
static_assert(std::ratio_less_v<ratio<1, 3>, ratio<1, 2>> && !std::ratio_less_v<ratio<1, 2>, ratio<1, 2>>);
static_assert(std::ratio_less_v<ratio<-1, 2>, ratio<1, 3>>);
static_assert(std::ratio_less_equal_v<ratio<1, 2>, ratio<2, 4>> && std::ratio_less_equal_v<ratio<1, 3>, ratio<1, 2>>);
static_assert(std::ratio_greater_v<ratio<3, 4>, ratio<2, 3>> && !std::ratio_greater_v<ratio<2, 3>, ratio<3, 4>>);
static_assert(std::ratio_greater_equal_v<ratio<2, 3>, ratio<4, 6>> && !std::ratio_greater_equal_v<ratio<-1>, ratio<0>>);
static_assert(std::is_base_of_v<std::true_type, std::ratio_less<ratio<1, 3>, ratio<1, 2>>>);
static_assert(std::is_base_of_v<std::false_type, std::ratio_greater<ratio<1, 3>, ratio<1, 2>>>);
