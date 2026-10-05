// [support.c.headers.other]/1: <inttypes.h> places in the global namespace each name <cinttypes>
// places in std ([cinttypes.syn]: imaxdiv_t; constexpr imaxabs and imaxdiv; strtoimax,
// strtoumax, wcstoimax, wcstoumax), and [cinttypes.syn]/1 <cinttypes> includes <cstdint>, so
// the <stdint.h> names (intmax_t ...) come along. The C23 binary-conversion macros PRIbN/PRIBN/
// SCNbN are defined. Only <inttypes.h> is included.
#include <inttypes.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::imaxdiv_t;
using ::int32_t;
using ::intmax_t;
using ::strtoimax;
using ::strtoumax;
using ::uintmax_t;
using ::wcstoimax;
using ::wcstoumax;

static_assert(same<decltype(::imaxabs(1)), ::intmax_t>);
static_assert(same<decltype(::imaxdiv(1, 1)), ::imaxdiv_t>);
static_assert(::imaxabs(-9) == 9);
static_assert(::imaxdiv(-17, 5).quot == -3 && ::imaxdiv(-17, 5).rem == -2);
static_assert(same<decltype(::strtoumax("", nullptr, 10)), ::uintmax_t>);

constexpr char d32[] = PRId32;
constexpr char b32[] = PRIb32;
constexpr char B64[] = PRIB64;
constexpr char sb8[] = SCNb8;
constexpr char bmax[] = PRIbMAX;
static_assert(d32[sizeof d32 - 2] == 'd' && b32[sizeof b32 - 2] == 'b' && B64[sizeof B64 - 2] == 'B');
static_assert(sb8[sizeof sb8 - 2] == 'b' && bmax[sizeof bmax - 2] == 'b');

int main() { return 0; }
