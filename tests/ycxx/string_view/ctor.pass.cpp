// [string.view.cons]: default (size_ == 0, data_ == nullptr); (const charT*) sets size_ to
// traits::length(str); "basic_string_view(nullptr_t) = delete;"; (const charT*, size_type);
// (It, End) with "Constraints: It satisfies contiguous_iterator. End satisfies
// sized_sentinel_for<It>. is_same_v<iter_value_t<It>, charT> is true.
// is_convertible_v<End, size_type> is false."
#include <string_view>
#include <array>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_constructible_v<std::string_view, std::nullptr_t>);
static_assert(std::is_convertible_v<const char*, std::string_view>);
static_assert(std::is_convertible_v<const char (&)[4], std::string_view>);
static_assert(std::is_constructible_v<std::string_view, const char*, std::size_t>);
static_assert(std::is_constructible_v<std::string_view, const char*, const char*>);
static_assert(std::is_constructible_v<std::string_view, char*, char*>);
static_assert(!std::is_constructible_v<std::string_view, const wchar_t*, const wchar_t*>);
static_assert(!std::is_constructible_v<std::string_view, const signed char*, const signed char*>);
static_assert(!std::is_constructible_v<std::string_view, const char*, const wchar_t*>);
static_assert(!std::is_constructible_v<std::string_view, const wchar_t*>);
static_assert(std::is_constructible_v<std::string_view, std::array<char, 3>::const_iterator,
                                      std::array<char, 3>::const_iterator>);

constexpr bool test() {
  std::string_view d;
  if (d.size() != 0 || d.data() != nullptr || !d.empty()) return false;
  const char* lit = "hello";
  std::string_view a(lit);
  if (a.size() != 5 || a.data() != lit) return false;
  std::string_view b(lit, 3);
  if (b.size() != 3 || b.data() != lit) return false;
  std::string_view c(lit + 1, lit + 4);
  if (c.size() != 3 || c.data() != lit + 1) return false;
  std::array<char, 3> arr{'x', 'y', 'z'};
  std::string_view e(arr.begin(), arr.end());
  if (e.size() != 3 || e.data() != arr.data()) return false;
  const char emb[] = {'a', '\0', 'b'};
  std::string_view f(emb, 3);  // embedded nulls are kept
  if (f.size() != 3 || f[2] != 'b') return false;
  std::string_view g = "with\0null";  // traits::length stops at the first null
  if (g.size() != 4) return false;
  std::u32string_view u = U"\U0001F600x";
  if (u.size() != 2) return false;
  std::string_view copy = a;
  if (copy.data() != a.data() || copy.size() != a.size()) return false;
  copy = b;
  if (copy.size() != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
