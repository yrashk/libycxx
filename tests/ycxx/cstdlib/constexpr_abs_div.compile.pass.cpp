// [cstdlib.syn] ([c.math.abs]; P0533R9, C++23): the integer absolute-value and division
// functions are constexpr:
//   constexpr int abs(int j); constexpr long int abs(long int j); constexpr long long int abs(long long int j);
//   constexpr long int labs(long int j); constexpr long long int llabs(long long int j);
//   constexpr div_t div(int numer, int denom);
//   constexpr ldiv_t div(long int numer, long int denom);
//   constexpr lldiv_t div(long long int numer, long long int denom);
//   constexpr ldiv_t ldiv(long int numer, long int denom);
//   constexpr lldiv_t lldiv(long long int numer, long long int denom);
// with the C semantics (ISO C 7.24.6.2: quot truncated toward zero, quot * denom + rem == numer).
#include <cstdlib>
#include <type_traits>

static_assert(std::abs(-5) == 5 && std::abs(-5L) == 5L && std::abs(-5LL) == 5LL);
static_assert(std::labs(-6L) == 6L && std::llabs(-6LL) == 6LL);

static_assert(std::is_same_v<decltype(std::div(1, 1)), std::div_t>);
static_assert(std::is_same_v<decltype(std::div(1L, 1L)), std::ldiv_t>);
static_assert(std::is_same_v<decltype(std::div(1LL, 1LL)), std::lldiv_t>);

constexpr std::div_t a = std::div(-7, 2);
static_assert(a.quot == -3 && a.rem == -1);
constexpr std::div_t b = std::div(7, -2);
static_assert(b.quot == -3 && b.rem == 1);
constexpr std::ldiv_t c = std::div(-9L, 4L);
static_assert(c.quot == -2 && c.rem == -1);
constexpr std::lldiv_t d = std::div(9LL, -4LL);
static_assert(d.quot == -2 && d.rem == 1);
constexpr std::ldiv_t e = std::ldiv(17L, 5L);
static_assert(e.quot == 3 && e.rem == 2);
constexpr std::lldiv_t f = std::lldiv(-17LL, -5LL);
static_assert(f.quot == 3 && f.rem == -2);
