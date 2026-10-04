// [string.view.iterators]: begin()/cbegin() "if !empty(), addressof(*begin()) == data_";
// end()/cend() return begin() + size(); rbegin()/crbegin() is const_reverse_iterator(end());
// rend()/crend() is const_reverse_iterator(begin()). All noexcept.
#include <string_view>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
static_assert(noexcept(std::declval<const SV&>().begin()));
static_assert(noexcept(std::declval<const SV&>().end()));
static_assert(noexcept(std::declval<const SV&>().cbegin()));
static_assert(noexcept(std::declval<const SV&>().cend()));
static_assert(noexcept(std::declval<const SV&>().rbegin()));
static_assert(noexcept(std::declval<const SV&>().rend()));
static_assert(noexcept(std::declval<const SV&>().crbegin()));
static_assert(noexcept(std::declval<const SV&>().crend()));
static_assert(std::is_same_v<decltype(std::declval<const SV&>().begin()), SV::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().rbegin()), SV::const_reverse_iterator>);
static_assert(std::is_same_v<decltype(*std::declval<const SV&>().begin()), const char&>);

constexpr bool test() {
  const char* p = "hello";
  SV s(p);
  if (std::addressof(*s.begin()) != p) return false;
  if (s.end() != s.begin() + 5 || s.cend() != s.cbegin() + 5) return false;
  if (std::to_address(s.begin()) != p) return false;
  if (*s.rbegin() != 'o' || *(s.rend() - 1) != 'h') return false;
  if (s.rbegin().base() != s.end() || s.crend().base() != s.begin()) return false;
  int n = 0;
  for (char c : s) n += c == 'l';
  if (n != 2) return false;
  SV e;
  if (e.begin() != e.end() || e.rbegin() != e.rend()) return false;
  // free begin/end and swap are available from <string_view> ([string.view.synop]/1)
  if (std::begin(s) != s.begin() || std::size(s) != 5 || std::data(s) != p) return false;
  SV a("a"), b("bb");
  std::swap(a, b);
  if (a.size() != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
