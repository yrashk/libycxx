// [string.view.ops]/20-28: starts_with / ends_with / contains for basic_string_view, charT
// and const charT*. The string_view and charT overloads are noexcept. contains(x) is
// "Equivalent to: return find(x) != npos;".
// COUNTERPART: libcxx:strings/string.view/string.view.template/(starts|ends)_with..*.pass.cpp
#include <string_view>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
static_assert(noexcept(std::declval<const SV&>().starts_with(SV())));
static_assert(noexcept(std::declval<const SV&>().starts_with('a')));
static_assert(noexcept(std::declval<const SV&>().ends_with(SV())));
static_assert(noexcept(std::declval<const SV&>().ends_with('a')));
static_assert(noexcept(std::declval<const SV&>().contains(SV())));
static_assert(noexcept(std::declval<const SV&>().contains('a')));

constexpr bool test() {
  SV s("hello world");
  if (!s.starts_with(SV("hello")) || s.starts_with(SV("world")) || !s.starts_with(SV())) return false;
  if (!s.starts_with('h') || s.starts_with('e')) return false;
  if (!s.starts_with("hel") || s.starts_with("hello world!")) return false;
  if (!s.ends_with(SV("world")) || s.ends_with(SV("hello")) || !s.ends_with(SV())) return false;
  if (!s.ends_with('d') || s.ends_with('l')) return false;
  if (!s.ends_with("rld") || s.ends_with("xhello world")) return false;
  if (!s.contains(SV("o w")) || s.contains(SV("wx")) || !s.contains(SV())) return false;
  if (!s.contains('w') || s.contains('z')) return false;
  if (!s.contains("lo") || s.contains("hello world hello")) return false;
  SV e;
  if (e.starts_with('a') || e.ends_with('a') || e.contains('a')) return false;
  if (!e.starts_with("") || !e.ends_with("") || !e.contains("")) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
