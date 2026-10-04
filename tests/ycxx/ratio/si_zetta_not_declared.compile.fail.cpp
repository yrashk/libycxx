// [ratio.si]/1: "if either of the constants is not representable by intmax_t, the typedef is
// not declared." With a 64-bit intmax_t, 10^21 is not representable, so std::zetta does not
// exist.
#include <ratio>
#include <cstdint>

static_assert(sizeof(std::intmax_t) == 8, "this test assumes a 64-bit intmax_t");
using Z = std::zetta;
