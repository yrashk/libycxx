// [string.view.capacity]/2: constexpr size_type max_size() const noexcept; "Returns: The
// largest possible number of char-like objects that can be referred to by a
// basic_string_view." A view refers to objects of a single array, so max_size() objects of
// type charT must fit in the address space: max_size() * sizeof(charT) cannot exceed the
// largest size_t. max_size() is a property of the type, independent of the viewed sequence.
#include <string_view>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class C>
constexpr bool test() {
  using SV = std::basic_string_view<C>;
  static_assert(noexcept(std::declval<const SV&>().max_size()));
  static_assert(std::is_same_v<decltype(std::declval<const SV&>().max_size()), typename SV::size_type>);
  constexpr std::size_t m = SV().max_size();
  static_assert(m > 0);
  static_assert(m <= std::numeric_limits<std::size_t>::max() / sizeof(C));
  const C s[] = {C('a'), C('b'), C(0)};
  if (SV(s).max_size() != m || SV(s, 1).max_size() != m) return false;
  return true;
}

static_assert(test<char>());
static_assert(test<wchar_t>());
static_assert(test<char8_t>());
static_assert(test<char16_t>());
static_assert(test<char32_t>());

int main() {
  CHECK(test<char>());
  CHECK(std::string_view().max_size() >= 0x10000);  // more than a toy limit
  CHECK(std::u32string_view().max_size() >= 0x10000);
  return 0;
}
