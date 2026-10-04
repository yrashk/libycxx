// [ratio.ratio]/1: "If ... the absolute values of either of the template arguments N and D is
// not representable by type intmax_t, the program is ill-formed." (Note 1: "This excludes the
// most negative value.")
#include <ratio>
#include <cstdint>

constexpr auto n = std::ratio<INTMAX_MIN, 1>::num;
