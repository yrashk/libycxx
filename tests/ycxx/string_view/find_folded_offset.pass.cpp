// [string.view.find]/2-4, [char.traits.require] (find, compare), [alg.find]: the results do not
// depend on whether the call is evaluated at run time or folded by the compiler. Each constexpr
// function below is called with constant arguments in a context that is not manifestly
// constant-evaluated (the initializer of a volatile object), which a compiler may fold; the
// strings are a literal seen at an offset, where GCC 16.2's constant evaluation of
// __builtin_memchr and __builtin_memcmp (find, compare, std::find, std::search) counts the offset
// twice (it gave 8 for find('c', 3) of "abcabcab"; the -O2 job of the nightly found it in
// string_view/find.pass.cpp).
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include "check.hpp"

using SV = std::string_view;

constexpr std::size_t find_char() { return SV("abcabcab").find('c', 3); }                 // memchr
constexpr std::size_t find_sv() { return SV("abcabcab").find(SV("ab"), 1); }              // memchr, memcmp
constexpr int compare_tail() { return SV("xxabcab").substr(4).compare(SV("cab")); }        // memcmp
constexpr std::size_t traits_find() {
  const char* s = "abcabcab";
  const char* p = std::char_traits<char>::find(s + 3, 5, 'c');
  return p ? static_cast<std::size_t>(p - s) : 99;
}
constexpr int traits_compare() { return std::char_traits<char>::compare(&"xxabcab"[2], "abcab", 5); }
constexpr std::ptrdiff_t algo_find() {
  const char* s = "abcabcab";
  return std::find(s + 3, s + 8, 'c') - s;
}
constexpr std::ptrdiff_t ranges_find() {
  const char* s = "abcabcab";
  return std::ranges::find(s + 3, s + 8, 'c') - s;
}

constexpr std::ptrdiff_t algo_search() {
  const char* s = "abcabcab";
  const char* n = "xcab";
  return std::search(s + 3, s + 8, n + 1, n + 4) - s;  // memchr, memcmp
}

static_assert(algo_search() == 5);
static_assert(find_char() == 5 && find_sv() == 3 && compare_tail() == 0 && traits_find() == 5 &&
              traits_compare() == 0 && algo_find() == 5 && ranges_find() == 5);

int main() {
  volatile std::size_t a = find_char(), b = find_sv(), d = traits_find();
  volatile int c = compare_tail(), e = traits_compare();
  volatile std::ptrdiff_t f = algo_find(), g = ranges_find(), h = algo_search();
  CHECK(a == 5);
  CHECK(b == 3);
  CHECK(c == 0);
  CHECK(d == 5);
  CHECK(e == 0);
  CHECK(f == 5);
  CHECK(g == 5);
  CHECK(h == 5);
  return 0;
}
