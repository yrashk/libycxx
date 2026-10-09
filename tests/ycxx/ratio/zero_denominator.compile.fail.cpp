// EXPECT-ERROR: error: static assertion failed[^\n]*\[ratio\.ratio\]/1: the denominator of std::ratio must not be zero
// [ratio.ratio]/1: "If the template argument D is zero ... the program is ill-formed."
#include <ratio>

constexpr auto n = std::ratio<1, 0>::num;
