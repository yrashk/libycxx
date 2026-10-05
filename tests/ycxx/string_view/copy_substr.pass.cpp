// [string.view.ops]/1-10: copy(s, n, pos): "Let rlen be the smaller of n and size() - pos."
// "Equivalent to traits::copy(s, data() + pos, rlen)." "Returns: rlen." "Throws:
// out_of_range if pos > size()." substr(pos, n) and (C++26) subview(pos, n): "Returns:
// basic_string_view(data() + pos, rlen)." "Throws: out_of_range if pos > size()."
// REQUIRES: exceptions
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
static_assert(std::is_same_v<decltype(std::declval<const SV&>().substr()), SV>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().subview()), SV>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().copy(nullptr, 0)), SV::size_type>);

constexpr bool test() {
  const char* p = "abcdef";
  SV s(p);
  SV a = s.substr(2);
  if (a.data() != p + 2 || a.size() != 4) return false;
  SV b = s.substr(1, 3);
  if (b != "bcd" || b.data() != p + 1) return false;
  SV c = s.substr(4, 100);
  if (c != "ef") return false;
  SV d = s.substr(6);
  if (!d.empty() || d.data() != p + 6) return false;
  if (s.substr() != s) return false;
  SV e = s.subview(1, 2);
  if (e != "bc" || e.data() != p + 1) return false;
  if (s.subview(3) != "def" || s.subview() != s) return false;

  char buf[8] = {};
  auto n = s.copy(buf, 3, 2);
  if (n != 3 || buf[0] != 'c' || buf[2] != 'e' || buf[3] != '\0') return false;
  n = s.copy(buf, 100);
  if (n != 6 || buf[5] != 'f') return false;
  n = s.copy(buf, 5, 6);
  if (n != 0) return false;
  return true;
}
static_assert(test());

template <class F>
bool throws(F f) {
  try {
    f();
  } catch (const std::out_of_range&) {
    return true;
  }
  return false;
}

int main() {
  CHECK(test());
  SV s("abc");
  char buf[4];
  CHECK(throws([&] { (void)s.substr(4); }));
  CHECK(throws([&] { (void)s.subview(4, 0); }));
  CHECK(throws([&] { (void)s.copy(buf, 1, 4); }));
  CHECK(!throws([&] { (void)s.substr(3); }));
  CHECK(!throws([&] { (void)s.copy(buf, 1, 3); }));
  return 0;
}
