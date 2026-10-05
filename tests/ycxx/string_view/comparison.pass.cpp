// [string.view.comparison]: operator== returns lhs.compare(rhs) == 0; operator<=> returns
// static_cast<R>(lhs.compare(rhs) <=> 0) where "R denote[s] the type
// traits::comparison_category if that qualified-id is valid and denotes a type, otherwise R is
// weak_ordering". Both are noexcept and take type_identity_t<basic_string_view> for one
// argument, so objects implicitly convertible to the string view compare too (Note 1).
// COUNTERPART: libcxx:strings/string.view/string.view.comparison/.*.pass.cpp
#include <string_view>
#include <compare>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

// Minimal character traits without comparison_category (char_traits is not required here).
struct PlainTraits {
  using char_type = char;
  using int_type = int;
  static constexpr bool eq(char a, char b) noexcept { return a == b; }
  static constexpr bool lt(char a, char b) noexcept { return a < b; }
  static constexpr int compare(const char* a, const char* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (lt(a[i], b[i])) return -1;
      if (lt(b[i], a[i])) return 1;
    }
    return 0;
  }
  static constexpr std::size_t length(const char* s) {
    std::size_t n = 0;
    while (s[n]) ++n;
    return n;
  }
  static constexpr const char* find(const char* s, std::size_t n, const char& c) {
    for (std::size_t i = 0; i < n; ++i)
      if (eq(s[i], c)) return s + i;
    return nullptr;
  }
  static constexpr char* copy(char* d, const char* s, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) d[i] = s[i];
    return d;
  }
  static constexpr char* move(char* d, const char* s, std::size_t n) { return copy(d, s, n); }
  static constexpr char* assign(char* d, std::size_t n, char c) {
    for (std::size_t i = 0; i < n; ++i) d[i] = c;
    return d;
  }
  static constexpr void assign(char& d, const char& c) noexcept { d = c; }
  static constexpr char to_char_type(int c) noexcept { return static_cast<char>(c); }
  static constexpr int to_int_type(char c) noexcept { return static_cast<unsigned char>(c); }
  static constexpr bool eq_int_type(int a, int b) noexcept { return a == b; }
  static constexpr int eof() noexcept { return -1; }
  static constexpr int not_eof(int c) noexcept { return c == -1 ? 0 : c; }
};
struct PartialTraits : PlainTraits {
  using comparison_category = std::partial_ordering;
};

using SV = std::string_view;
using PV = std::basic_string_view<char, PlainTraits>;
using QV = std::basic_string_view<char, PartialTraits>;

static_assert(std::is_same_v<decltype(SV() <=> SV()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::wstring_view() <=> std::wstring_view()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(PV() <=> PV()), std::weak_ordering>);
static_assert(std::is_same_v<decltype(QV() <=> QV()), std::partial_ordering>);
static_assert(std::is_same_v<decltype(SV() == SV()), bool>);
static_assert(noexcept(SV() == SV()));
static_assert(noexcept(SV() <=> SV()));

struct Conv {
  constexpr operator SV() const { return "mid"; }
};

constexpr bool test() {
  SV a("abc"), b("abd"), c("abc");
  if (!(a == c) || a == b || !(a != b)) return false;
  if (!(a < b) || !(b > a) || !(a <= c) || !(a >= c)) return false;
  if ((a <=> b) != std::strong_ordering::less || (a <=> c) != std::strong_ordering::equal) return false;
  if ((SV("ab") <=> SV("abc")) != std::strong_ordering::less) return false;
  // mixed with const char* and convertible types, in both orders
  if (!(a == "abc") || !("abc" == a) || !(a < "b") || !("b" > a)) return false;
  if (!(Conv{} == SV("mid")) || !(SV("mid") == Conv{}) || !(Conv{} > a) || !(a < Conv{})) return false;
  if ((PV("x") <=> PV("y")) != std::weak_ordering::less) return false;
  if (!(PV("x") == PV("x"))) return false;
  if ((QV("y") <=> QV("x")) != std::partial_ordering::greater) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
