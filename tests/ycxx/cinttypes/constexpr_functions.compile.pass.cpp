// [cinttypes.syn]: "constexpr intmax_t imaxabs(intmax_t j);" and "constexpr imaxdiv_t
// imaxdiv(intmax_t numer, intmax_t denom);" (ISO C 7.8.2.1-2: the absolute value; quot and rem
// as for / and %, the quotient truncated toward zero).
#include <cinttypes>

static_assert(std::imaxabs(-42) == 42 && std::imaxabs(7) == 7 && std::imaxabs(INTMAX_MIN + 1) == INTMAX_MAX);
static_assert(std::imaxdiv(17, 5).quot == 3 && std::imaxdiv(17, 5).rem == 2);
static_assert(std::imaxdiv(-17, 5).quot == -3 && std::imaxdiv(-17, 5).rem == -2);
static_assert(std::imaxdiv(17, -5).quot == -3 && std::imaxdiv(17, -5).rem == 2);
constexpr std::imaxdiv_t d = std::imaxdiv(INTMAX_MAX, 2);
static_assert(d.quot == INTMAX_MAX / 2 && d.rem == 1);
