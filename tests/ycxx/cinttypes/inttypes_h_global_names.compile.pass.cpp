// [support.c.headers.other]/1: <inttypes.h> places in the global namespace each name <cinttypes>
// places in std ([cinttypes.syn]: imaxdiv_t; constexpr imaxabs and imaxdiv; strtoimax,
// strtoumax, wcstoimax, wcstoumax), and [cinttypes.syn]/1 <cinttypes> includes <cstdint>, so
// the <stdint.h> names (intmax_t ...) come along. The C23 binary-conversion macros PRIbN and
// SCNbN are defined; PRIBN only where fprintf supports the B conversion ([cinttypes.syn]/2: "Each
// of the PRIB macros listed in this subclause is defined if and only if fprintf supports the B
// conversion specifier"), so a PRIBN macro is checked when it is defined. Only <inttypes.h> is
// included.
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
#ifdef PRIB64
constexpr char B64[] = PRIB64;
#endif
constexpr char sb8[] = SCNb8;
constexpr char bmax[] = PRIbMAX;
static_assert(d32[sizeof d32 - 2] == 'd' && b32[sizeof b32 - 2] == 'b');
#ifdef PRIB64
static_assert(B64[sizeof B64 - 2] == 'B');
#endif
static_assert(sb8[sizeof sb8 - 2] == 'b' && bmax[sizeof bmax - 2] == 'b');

int main() { return 0; }
