// [span.cons]/15-17: span(R&& r) accepts a contiguous, sized range whose reference type's
// array converts to element_type(*)[], requiring borrowed_range<R> or a const element_type.
// basic_string is a contiguous container ([basic.string.general]/2), so an lvalue string
// converts implicitly to span<charT> and span<const charT>; a const string or an rvalue only
// to span<const charT>. The span views data() with size() elements — the null terminator is
// not part of it.
#include <string>
#include <span>
#include <utility>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_convertible_v<std::string&, std::span<char>>);
static_assert(std::is_convertible_v<std::string&, std::span<const char>>);
static_assert(std::is_convertible_v<const std::string&, std::span<const char>>);
static_assert(std::is_convertible_v<std::string&&, std::span<const char>>);
static_assert(!std::is_constructible_v<std::span<char>, const std::string&>);
static_assert(!std::is_constructible_v<std::span<char>, std::string&&>);
static_assert(!std::is_constructible_v<std::span<wchar_t>, std::string&>);
static_assert(std::is_convertible_v<std::u32string&, std::span<char32_t>>);

constexpr bool test() {
  std::string s = "hello";
  std::span<char> sp = s;
  if (sp.data() != s.data() || sp.size() != 5) return false;
  sp[0] = 'J';
  if (s != "Jello") return false;
  std::span<const char> cs = std::as_const(s);
  if (cs.size() != s.size() || cs.back() != 'o') return false;
  std::string big(100, 'x');
  std::span<char> bs = big;
  bs[99] = 'y';
  return big.back() == 'y' && bs.size() == 100;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
