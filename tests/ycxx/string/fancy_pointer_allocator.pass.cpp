// basic_string with an allocator whose pointer is a class type (support/fancy_ptr.hpp).
// [basic.string.general]: pointer is allocator_traits<Allocator>::pointer and const_pointer
// allocator_traits<Allocator>::const_pointer; data() and c_str() still return (const) charT*
// ([string.accessors]: "Returns: A pointer p such that p + i == addressof(operator[](i))");
// [string.require]/3: basic_string uses the allocator for its storage
// ([allocator.requirements.general]: allocate returns XX::pointer). Every member is constexpr
// ([basic.string.general]), so a string with this allocator works in constant evaluation too.
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "fancy_ptr.hpp"
#include "check.hpp"

template <class C>
using FS = std::basic_string<C, std::char_traits<C>, FancyAlloc<C>>;
using S = FS<char>;

static_assert(std::is_same_v<S::pointer, FancyPtr<char>>);
static_assert(std::is_same_v<S::const_pointer, FancyPtr<const char>>);
static_assert(std::is_same_v<decltype(std::declval<S&>().data()), char*>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().data()), const char*>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().c_str()), const char*>);
static_assert(std::is_same_v<S::allocator_type, FancyAlloc<char>>);

template <class C>
constexpr bool test_char_type() {
  using T = FS<C>;
  const C lit[] = {C('a'), C('b'), C('c'), C(0)};
  T s(lit);
  if (s.size() != 3 || s[0] != C('a') || s.c_str()[3] != C(0)) return false;
  for (int i = 0; i < 50; ++i) s.push_back(C('x'));  // beyond any small buffer
  if (s.size() != 53 || s.back() != C('x') || s.data()[53] != C(0)) return false;
  s.insert(1, 5, C('y'));
  if (s[1] != C('y') || s[6] != C('b') || s.size() != 58) return false;
  s.erase(1, 5);
  if (s.compare(0, 3, lit) != 0) return false;
  s.replace(0, 1, 10, C('z'));
  if (s.size() != 62 || s[9] != C('z') || s[10] != C('b')) return false;
  T t = s;
  if (t != s || t.data() == s.data()) return false;
  T u = std::move(t);
  if (u != s) return false;
  u.swap(s);
  if (u != s) return false;
  s.resize(2);
  s.shrink_to_fit();
  if (s.size() != 2 || s.c_str()[2] != C(0)) return false;
  s.clear();
  if (!s.empty() || s.data()[0] != C(0)) return false;
  s.assign(100, C('q'));
  s.resize_and_overwrite(120, [](C* p, std::size_t n) {
    for (std::size_t i = 100; i < n; ++i) p[i] = C('r');
    return n;
  });
  if (s.size() != 120 || s[99] != C('q') || s[100] != C('r') || s.c_str()[120] != C(0)) return false;
  std::size_t count = 0;
  for (auto it = s.begin(); it != s.end(); ++it) count += (*it == C('r'));
  if (count != 20) return false;
  if (s.rbegin()[0] != C('r') || s.end() - s.begin() != 120) return false;
  if (s.find(C('r')) != 100 || s.rfind(C('q')) != 99) return false;
  T cat = s.substr(98, 4) + s.substr(0, 1);
  if (cat.size() != 5 || cat[2] != C('r') || cat[4] != C('q')) return false;
  std::basic_string_view<C> sv = cat;
  if (sv.size() != 5 || sv.data() != cat.data()) return false;
  return true;
}

int main() {
  static_assert(test_char_type<char>());
  static_assert(test_char_type<wchar_t>());
  static_assert(test_char_type<char8_t>());
  static_assert(test_char_type<char16_t>());
  static_assert(test_char_type<char32_t>());
  CHECK(test_char_type<char>());
  CHECK(test_char_type<wchar_t>());
  CHECK(test_char_type<char8_t>());
  CHECK(test_char_type<char16_t>());
  CHECK(test_char_type<char32_t>());

  // data() points into the allocated storage: one past the last element is p + size()
  S s(40, 'a');
  CHECK(&s[39] == s.data() + 39);
  CHECK(&*s.begin() == s.data());
  return 0;
}
