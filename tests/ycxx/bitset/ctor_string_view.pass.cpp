// [bitset.cons]/3-7 (C++26, P2697): template<class charT, class traits> constexpr explicit
// bitset(basic_string_view<charT, traits> str, size_type pos = 0, size_type n = npos,
// charT zero = charT('0'), charT one = charT('1')); "Throws: out_of_range if
// pos > str.size() or invalid_argument if any of the rlen characters in str beginning at
// position pos is other than zero or one."
#include <bitset>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_constructible_v<std::bitset<8>, std::string_view>);
static_assert(!std::is_convertible_v<std::string_view, std::bitset<8>>);
static_assert(std::is_constructible_v<std::bitset<8>, std::u16string_view, std::size_t, std::size_t>);

constexpr bool test() {
  using namespace std::string_view_literals;
  std::bitset<8> a("0110"sv);
  if (a.to_ulong() != 6) return false;
  std::bitset<8> b("xx101"sv, 2);
  if (b.to_ulong() != 5) return false;
  std::bitset<8> c("xx1011yy"sv, 2, 3);
  if (c.to_ulong() != 5) return false;
  std::bitset<8> d("ab"sv, 0, 2, 'a', 'b');
  if (d.to_ulong() != 1) return false;
  std::bitset<8> e("111"sv, 3);  // pos == size(): empty, no throw
  if (e.any()) return false;
  // a string_view need not be null-terminated
  std::bitset<8> f(std::string_view("1111", 2));
  if (f.to_ulong() != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  bool caught = false;
  try {
    std::bitset<8> x(std::string_view("101"), 4);
  } catch (const std::out_of_range&) {
    caught = true;
  }
  CHECK(caught);
  caught = false;
  try {
    std::bitset<8> x(std::string_view("1021"));
  } catch (const std::invalid_argument&) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
