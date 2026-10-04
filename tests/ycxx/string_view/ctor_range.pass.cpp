// [string.view.cons]/11-14: template<class R> constexpr explicit basic_string_view(R&& r);
// "Constraints: remove_cvref_t<R> is not the same type as basic_string_view, R models
// ranges::contiguous_range and ranges::sized_range, is_same_v<ranges::range_value_t<R>,
// charT> is true, is_convertible_v<R, const charT*> is false, and
// d.operator ::std::basic_string_view<charT, traits>() is not a valid expression."
#include <string_view>
#include <array>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct Buf {
  char data_[4] = {'a', 'b', 'c', 'd'};
  constexpr const char* begin() const { return data_; }
  constexpr const char* end() const { return data_ + 4; }
  constexpr const char* data() const { return data_; }
  constexpr std::size_t size() const { return 4; }
};
// Has a conversion operator to string_view: the range constructor is excluded (12.5), so
// conversion goes through the operator.
struct WithConv {
  char data_[2] = {'x', 'y'};
  constexpr const char* begin() const { return data_; }
  constexpr const char* end() const { return data_ + 2; }
  constexpr operator std::string_view() const { return std::string_view(data_, 1); }
};
struct WideBuf {
  wchar_t d[1] = {L'w'};
  const wchar_t* begin() const { return d; }
  const wchar_t* end() const { return d + 1; }
};

static_assert(std::is_constructible_v<std::string_view, Buf&>);
static_assert(std::is_constructible_v<std::string_view, const Buf&>);
static_assert(std::is_constructible_v<std::string_view, Buf>);
static_assert(!std::is_convertible_v<Buf&, std::string_view>);  // explicit
static_assert(std::is_constructible_v<std::string_view, std::array<char, 2>&>);
static_assert(!std::is_convertible_v<std::array<char, 2>&, std::string_view>);
static_assert(!std::is_constructible_v<std::string_view, WideBuf&>);
static_assert(!std::is_constructible_v<std::string_view, std::array<signed char, 2>&>);
static_assert(std::is_convertible_v<WithConv, std::string_view>);

constexpr bool test() {
  Buf b;
  std::string_view s(b);
  if (s.data() != b.data_ || s.size() != 4) return false;
  std::array<char, 3> arr{'1', '2', '3'};
  std::string_view a(arr);
  if (a.size() != 3 || a.data() != arr.data()) return false;
  WithConv w;
  std::string_view v(w);  // uses the conversion operator: size 1
  if (v.size() != 1) return false;
  // a char array is convertible to const char*, so the pointer constructor (strlen) is used
  const char ca[6] = {'a', 'b', '\0', 'c', 'd', '\0'};
  std::string_view z(ca);
  if (z.size() != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
