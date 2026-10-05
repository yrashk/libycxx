// [string.substr]/1: substr(pos, n) const & is basic_string(*this, pos, n); /2:
// substr(pos, n) && is basic_string(std::move(*this), pos, n); both throw out_of_range when
// pos > size() ([string.cons]/6). /3: subview(pos, n) returns
// basic_string_view(*this).subview(pos, n), a view into the string's own characters.
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<const std::string&>().substr()), std::string>);
static_assert(std::is_same_v<decltype(std::declval<std::string&&>().substr()), std::string>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>().subview()), std::string_view>);
static_assert(std::is_same_v<decltype(std::declval<const std::wstring&>().subview(1, 2)), std::wstring_view>);

constexpr bool test() {
  const std::string s = "0123456789";
  if (s.substr() != s) return false;
  if (s.substr(3) != "3456789") return false;
  if (s.substr(3, 2) != "34") return false;
  if (s.substr(8, 100) != "89") return false;
  if (s.substr(10) != "") return false;
  if (std::string("abcdef").substr(2, 3) != "cde") return false;
  std::string m = "a long string so that a move could steal the buffer, perhaps";
  std::string r = std::move(m).substr(2, 4);
  if (r != "long") return false;
  std::string m2 = "xyz";
  if (std::move(m2).substr() != "xyz") return false;

  std::string_view v = s.subview(2, 3);
  if (v != "234" || v.data() != s.data() + 2) return false;
  if (s.subview() != "0123456789" || s.subview().data() != s.data()) return false;
  if (s.subview(7) != "789") return false;
  if (!s.subview(10).empty()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  const std::string s = "abc";
  int threw = 0;
  try { (void)s.substr(4); } catch (const std::out_of_range&) { ++threw; }
  try { (void)std::string("abc").substr(4, 1); } catch (const std::out_of_range&) { ++threw; }
  try { (void)s.subview(4); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 3);
  return 0;
}
