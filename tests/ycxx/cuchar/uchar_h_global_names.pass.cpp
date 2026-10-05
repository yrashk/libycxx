// [support.c.headers.other]/1: <uchar.h> places in the global namespace each name <cuchar> places
// in std ([cuchar.syn]: mbstate_t, size_t, mbrtoc8, c8rtomb, mbrtoc16, c16rtomb, mbrtoc32,
// c32rtomb; mbrtoc8/c8rtomb take char8_t, ISO C 7.30.2). Under the "C" locale plain ASCII
// converts to itself. Only <uchar.h> (and <locale.h> for setlocale) is included.
#include <uchar.h>
#include <locale.h>

#include "check.hpp"

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::mbstate_t;
using ::size_t;

static_assert(same<decltype(::mbrtoc8(nullptr, nullptr, 0, nullptr)), ::size_t>);
static_assert(same<decltype(::c8rtomb(nullptr, u8'a', nullptr)), ::size_t>);
static_assert(same<decltype(::mbrtoc16(nullptr, nullptr, 0, nullptr)), ::size_t>);
static_assert(same<decltype(::c16rtomb(nullptr, u'a', nullptr)), ::size_t>);
static_assert(same<decltype(::mbrtoc32(nullptr, nullptr, 0, nullptr)), ::size_t>);
static_assert(same<decltype(::c32rtomb(nullptr, U'a', nullptr)), ::size_t>);

int main() {
  ::setlocale(LC_ALL, "C");
  ::mbstate_t st{};
  char8_t c8 = 0;
  CHECK(::mbrtoc8(&c8, "A", 1, &st) == 1 && c8 == u8'A');
  char out[8] = {};
  CHECK(::c8rtomb(out, u8'B', &st) == 1 && out[0] == 'B');
  char16_t c16 = 0;
  CHECK(::mbrtoc16(&c16, "C", 1, &st) == 1 && c16 == u'C');
  char32_t c32 = 0;
  CHECK(::mbrtoc32(&c32, "D", 1, &st) == 1 && c32 == U'D');
  CHECK(::c32rtomb(out, U'E', &st) == 1 && out[0] == 'E');
  return 0;
}
