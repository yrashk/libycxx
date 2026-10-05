// [support.c.headers.other]/1: <stdlib.h> "behaves as if each name placed in the standard
// library namespace by the corresponding <cstdlib> header is placed within the global namespace
// scope" (the exceptions, [sf.cmath], lerp, byte and its operations, are not <cstdlib> names).
// So the C++ overload sets of [cstdlib.syn] are global too, with their return types and
// constexpr:
//   constexpr int abs(int); constexpr long abs(long); constexpr long long abs(long long);
//   constexpr floating-point-type abs(floating-point-type);   ([c.math.abs])
//   constexpr div_t div(int, int); ldiv_t div(long, long); lldiv_t div(long long, long long);
//   labs, llabs, ldiv, lldiv;
//   atexit / at_quick_exit for both C and C++ linkage handlers ([support.start.term]);
//   bsearch with const and non-const base, qsort, for both linkages ([alg.c.library]).
// Only <stdlib.h> is included. The C23 additions: cstdlib/stdlib_h_c23_global_names.
#include <stdlib.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

// Types and macros.
using ::div_t;
using ::ldiv_t;
using ::lldiv_t;
using ::size_t;
static_assert(EXIT_SUCCESS == 0 || EXIT_SUCCESS != EXIT_FAILURE);
static_assert(RAND_MAX >= 32767);
static_assert(same<decltype(MB_CUR_MAX), decltype(MB_CUR_MAX)>);

// Functions with their C signatures.
using ::_Exit;
using ::abort;
using ::aligned_alloc;
using ::at_quick_exit;
using ::atexit;
using ::atof;
using ::atoi;
using ::atol;
using ::atoll;
using ::calloc;
using ::exit;
using ::free;
using ::getenv;
using ::malloc;
using ::mblen;
using ::mbstowcs;
using ::mbtowc;
using ::quick_exit;
using ::rand;
using ::realloc;
using ::srand;
using ::strtod;
using ::strtof;
using ::strtol;
using ::strtold;
using ::strtoll;
using ::strtoul;
using ::strtoull;
using ::system;
using ::wcstombs;
using ::wctomb;

static_assert(same<decltype(::strtof("", nullptr)), float>);
static_assert(same<decltype(::strtold("", nullptr)), long double>);
static_assert(same<decltype(::strtoull("", nullptr, 10)), unsigned long long>);

// The C++ overloads of abs and div, constexpr.
static_assert(same<decltype(::abs(1)), int>);
static_assert(same<decltype(::abs(1L)), long>);
static_assert(same<decltype(::abs(1LL)), long long>);
static_assert(same<decltype(::abs(1.0f)), float>);
static_assert(same<decltype(::abs(1.0)), double>);
static_assert(same<decltype(::abs(1.0L)), long double>);
static_assert(same<decltype(::div(1, 1)), ::div_t>);
static_assert(same<decltype(::div(1L, 1L)), ::ldiv_t>);
static_assert(same<decltype(::div(1LL, 1LL)), ::lldiv_t>);

static_assert(::abs(-3) == 3 && ::abs(-3L) == 3L && ::abs(-3LL) == 3LL);
static_assert(::labs(-4L) == 4L && ::llabs(-4LL) == 4LL);
static_assert(::abs(-2.5) == 2.5 && ::abs(-2.5f) == 2.5f && ::abs(-2.5L) == 2.5L);
static_assert(::div(-7, 2).quot == -3 && ::div(-7, 2).rem == -1);
static_assert(::div(-7L, 2L).quot == -3L && ::div(7LL, -2LL).rem == 1LL);
static_assert(::ldiv(17L, 5L).rem == 2L && ::lldiv(-17LL, 5LL).quot == -3LL);

// atexit / at_quick_exit accept handlers of either language linkage.
extern "C" void c_handler() {}
void cxx_handler() {}
static_assert(same<decltype(::atexit(c_handler)), int> && same<decltype(::atexit(cxx_handler)), int>);
static_assert(same<decltype(::at_quick_exit(c_handler)), int> &&
              same<decltype(::at_quick_exit(cxx_handler)), int>);
static_assert(noexcept(::atexit(cxx_handler)) && noexcept(::at_quick_exit(c_handler)));

// bsearch: const base gives const void*, non-const gives void*; both linkages for qsort.
extern "C" int c_cmp(const void*, const void*) { return 0; }
int cxx_cmp(const void*, const void*) { return 0; }
const int carr[1] = {};
int arr[1] = {};
static_assert(same<decltype(::bsearch(carr, carr, 1, sizeof(int), c_cmp)), const void*>);
static_assert(same<decltype(::bsearch(carr, carr, 1, sizeof(int), cxx_cmp)), const void*>);
static_assert(same<decltype(::bsearch(carr, arr, 1, sizeof(int), cxx_cmp)), void*>);
static_assert(same<decltype(::qsort(arr, 1, sizeof(int), c_cmp)), void>);
static_assert(same<decltype(::qsort(arr, 1, sizeof(int), cxx_cmp)), void>);

int main() { return 0; }
