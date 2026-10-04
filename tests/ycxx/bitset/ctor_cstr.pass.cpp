// [bitset.cons]/8-9: template<class charT> constexpr explicit bitset(const charT* str,
// typename basic_string_view<charT>::size_type n = npos, charT zero = charT('0'),
// charT one = charT('1')); "Effects: As if by: bitset(n == npos ? basic_string_view<charT>(str)
// : basic_string_view<charT>(str, n), 0, n, zero, one)". [bitset.cons]/3-5: the effective
// length is min(n, size); "Character position pos + M - 1 corresponds to bit position zero";
// "If M < N, remaining bit positions are initialized to zero."
#include <bitset>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_constructible_v<std::bitset<4>, const char*>);
static_assert(!std::is_convertible_v<const char*, std::bitset<4>>);  // explicit
static_assert(std::is_constructible_v<std::bitset<4>, const wchar_t*>);
static_assert(std::is_constructible_v<std::bitset<4>, const char16_t*>);
static_assert(std::is_constructible_v<std::bitset<4>, const char32_t*>);
static_assert(std::is_constructible_v<std::bitset<4>, const char8_t*>);

constexpr bool test() {
  std::bitset<8> a("1011");
  if (a.to_ulong() != 0b1011) return false;

  std::bitset<4> b("110010");  // only the first N characters are used: "1100"
  if (b.to_ulong() != 0b1100) return false;

  std::bitset<8> c("101101", 3);  // n = 3: "101"
  if (c.to_ulong() != 0b101) return false;

  std::bitset<8> d("xyyx", 4, 'x', 'y');
  if (d.to_ulong() != 0b0110) return false;

  std::bitset<8> e(u"11");
  if (e.to_ulong() != 3) return false;
  std::bitset<8> f(U"10");
  if (f.to_ulong() != 2) return false;
  std::bitset<8> g(L"100");
  if (g.to_ulong() != 4) return false;
  std::bitset<8> h(u8"1");
  if (h.to_ulong() != 1) return false;

  std::bitset<8> empty("");
  if (empty.any()) return false;

  std::bitset<3> zero_len("111", 0);
  if (zero_len.any()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  bool caught = false;
  try {
    std::bitset<8> bad("10a1");
  } catch (const std::invalid_argument&) {
    caught = true;
  }
  CHECK(caught);
  caught = false;
  try {
    // the character beyond n is not examined
    std::bitset<8> ok("10a1", 2);
    CHECK(ok.to_ulong() == 2);
  } catch (...) {
    caught = true;
  }
  CHECK(!caught);
  caught = false;
  try {
    std::bitset<8> bad("0120", 4, '0', '2');  // '1' is now invalid
  } catch (const std::invalid_argument&) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
