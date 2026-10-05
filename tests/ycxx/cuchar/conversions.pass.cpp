// [cuchar.syn]: std::mbrtoc8, c8rtomb, mbrtoc16, c16rtomb, mbrtoc32, c32rtomb and mbstate_t,
// with the meaning of ISO C 7.30 (C23): mbrtoc16/mbrtoc32 return the number of bytes of the
// multibyte character, 0 for the null character, (size_t)-2 for an incomplete character,
// (size_t)-1 for an encoding error, and (size_t)-3 when a further code unit of an earlier
// character was stored without consuming input (the second UTF-16 surrogate; for mbrtoc8,
// every code unit after the first); c*rtomb store the multibyte character and return its
// length, or 0 for a code unit that does not complete a character (a high surrogate, or a
// non-final UTF-8 code unit). Under the "C.UTF-8" locale the multibyte encoding is UTF-8;
// under "C", plain ASCII converts to itself.
// COUNTERPART: libcxx:strings/c.strings/no_c8rtomb_mbrtoc8.verify.cpp
#include <cuchar>
#include <clocale>
#include <cstring>
#include <cwchar>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::mbrtoc8(nullptr, "", 0, nullptr)), std::size_t>);
static_assert(std::is_same_v<decltype(std::c8rtomb(nullptr, u8'a', nullptr)), std::size_t>);

constexpr std::size_t err = static_cast<std::size_t>(-1), incomplete = static_cast<std::size_t>(-2),
                      stored = static_cast<std::size_t>(-3);

static void ascii() {
  std::mbstate_t st{};
  char32_t c32 = 0;
  CHECK(std::mbrtoc32(&c32, "Az", 2, &st) == 1 && c32 == U'A');
  char16_t c16 = 0;
  CHECK(std::mbrtoc16(&c16, "z", 1, &st) == 1 && c16 == u'z');
  char8_t c8 = 0;
  CHECK(std::mbrtoc8(&c8, "q", 1, &st) == 1 && c8 == u8'q');
  CHECK(std::mbrtoc32(&c32, "", 1, &st) == 0 && c32 == 0);
  char out[8];
  CHECK(std::c32rtomb(out, U'x', &st) == 1 && out[0] == 'x');
  CHECK(std::c16rtomb(out, u'y', &st) == 1 && out[0] == 'y');
  CHECK(std::c8rtomb(out, u8'w', &st) == 1 && out[0] == 'w');
}

static void utf8() {
  const char* s = "\xC3\xA9\xF0\x9F\x98\x80\xE2\x82\xAC";  // U+00E9, U+1F600, U+20AC
  std::mbstate_t st{};
  char32_t c32 = 0;
  CHECK(std::mbrtoc32(&c32, s, 9, &st) == 2 && c32 == U'é');
  CHECK(std::mbrtoc32(&c32, s + 2, 7, &st) == 4 && c32 == U'\U0001F600');
  CHECK(std::mbrtoc32(&c32, s + 6, 1, &st) == incomplete);  // first byte of U+20AC
  CHECK(std::mbrtoc32(&c32, s + 7, 2, &st) == 2 && c32 == U'€');
  // UTF-16: a surrogate pair, the second unit returned with (size_t)-3
  st = {};
  char16_t c16 = 0;
  CHECK(std::mbrtoc16(&c16, s + 2, 4, &st) == 4 && c16 == 0xD83D);
  CHECK(std::mbrtoc16(&c16, s + 6, 3, &st) == stored && c16 == 0xDE00);
  CHECK(std::mbrtoc16(&c16, s + 6, 3, &st) == 3 && c16 == 0x20AC);
  // UTF-8 code units: the first consumes the bytes, the others are (size_t)-3
  st = {};
  char8_t c8 = 0;
  CHECK(std::mbrtoc8(&c8, s, 9, &st) == 2 && c8 == 0xC3);
  CHECK(std::mbrtoc8(&c8, s + 2, 7, &st) == stored && c8 == 0xA9);
  CHECK(std::mbrtoc8(&c8, s + 2, 7, &st) == 4 && c8 == 0xF0);
  for (unsigned char want : {0x9F, 0x98, 0x80}) CHECK(std::mbrtoc8(&c8, s + 6, 3, &st) == stored && c8 == want);
  // an encoding error
  st = {};
  CHECK(std::mbrtoc32(&c32, "\xFF", 1, &st) == err);
  // back to multibyte
  char out[8];
  st = {};
  CHECK(std::c32rtomb(out, U'\U0001F600', &st) == 4 && std::memcmp(out, s + 2, 4) == 0);
  CHECK(std::c16rtomb(out, 0xD83D, &st) == 0);  // high surrogate: nothing yet
  CHECK(std::c16rtomb(out, 0xDE00, &st) == 4 && std::memcmp(out, s + 2, 4) == 0);
  std::string acc;
  for (char8_t u : {char8_t(0xE2), char8_t(0x82), char8_t(0xAC)}) {
    std::size_t n = std::c8rtomb(out, u, &st);
    CHECK(n != err);
    acc.append(out, n);
  }
  CHECK(acc == "\xE2\x82\xAC");
}

int main() {
  CHECK(std::strcmp(std::setlocale(LC_ALL, nullptr), "C") == 0);
  ascii();
  if (std::setlocale(LC_ALL, "C.UTF-8") || std::setlocale(LC_ALL, "C.utf8")) {
    ascii();
    utf8();
  }
}
