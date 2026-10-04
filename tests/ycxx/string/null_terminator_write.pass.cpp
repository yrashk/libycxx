// [basic.string.general]/3: data() + size() points at a null terminator, and "The program has
// undefined behavior if the null terminator is modified to any value other than charT()" —
// so writing charT() to it is permitted and leaves the string unchanged. [string.access]/2:
// operator[](pos) returns data()[pos] for pos <= size(), so the non-const s[s.size()] is a
// charT& to the terminator. [string.accessors]/1: data() (non-const, charT*) returns
// to_address(begin()), so writes through data()[i] for i < size() modify the string.
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::string&>()[0]), char&>);

template <class S>
constexpr bool test(S s) {
  using C = typename S::value_type;
  const auto n = s.size();
  const S orig = s;
  s[n] = C();
  s.data()[n] = C();
  if (s != orig || s.size() != n || s[n] != C()) return false;
  C* p = s.data();
  for (std::size_t i = 0; i < n; ++i) p[i] = static_cast<C>('A' + i % 26);
  for (std::size_t i = 0; i < n; ++i)
    if (s[i] != static_cast<C>('A' + i % 26)) return false;
  if (s.c_str()[n] != C()) return false;
  // writing an embedded null through data() keeps the size
  if (n > 1) {
    s.data()[1] = C();
    if (s.size() != n || s[1] != C()) return false;
  }
  S e;
  e[0] = C();
  e.data()[0] = C();
  return e.empty() && e.c_str()[0] == C();
}

static_assert(test(std::string("abc")));
static_assert(test(std::string("a long string that has to live in allocated storage!")));
static_assert(test(std::u16string(u"xyz")));

int main() {
  CHECK(test(std::string("abc")));
  CHECK(test(std::string("a long string that has to live in allocated storage!")));
  CHECK(test(std::wstring(L"wide")));
  CHECK(test(std::u8string(u8"eight")));
  CHECK(test(std::u32string(U"a long string that has to live in allocated storage!")));
  return 0;
}
