// EXPECT-ERROR-GCC: error: 'quecto' in namespace 'std' does not name a type
// EXPECT-ERROR-CLANG: error: no type named 'quecto' in namespace 'std'
// [ratio.si]/1: quecto (10^-30) is declared only if 10^30 is representable by intmax_t; with a
// 64-bit intmax_t it is not, so std::quecto does not exist.
#include <ratio>
#include <cstdint>

static_assert(sizeof(std::intmax_t) == 8, "this test assumes a 64-bit intmax_t");
using Q = std::quecto;
