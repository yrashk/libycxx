// [string.view.capacity]: size(), length() return size_; max_size(); empty().
// [string.view.access]: operator[] returns data_[pos]; at(pos) "Throws: out_of_range if
// pos >= size()."; front() is data_[0]; back() is data_[size() - 1]; data().
// [string.view.modifiers]: remove_prefix, remove_suffix, swap (noexcept).
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
static_assert(noexcept(std::declval<const SV&>().size()));
static_assert(noexcept(std::declval<const SV&>().length()));
static_assert(noexcept(std::declval<const SV&>().max_size()));
static_assert(noexcept(std::declval<const SV&>().empty()));
static_assert(noexcept(std::declval<const SV&>().data()));
static_assert(noexcept(std::declval<SV&>().swap(std::declval<SV&>())));
static_assert(std::is_same_v<decltype(std::declval<const SV&>()[0]), const char&>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().at(0)), const char&>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().front()), const char&>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().back()), const char&>);
static_assert(std::is_same_v<decltype(std::declval<const SV&>().data()), const char*>);
static_assert(std::is_same_v<decltype(std::declval<SV&>().remove_prefix(1)), void>);

constexpr bool test() {
  const char* p = "abcdef";
  SV s(p);
  if (s.size() != 6 || s.length() != 6 || s.empty()) return false;
  if (s.max_size() < s.size()) return false;
  if (s[0] != 'a' || &s[5] != p + 5 || s.at(2) != 'c' || &s.at(3) != p + 3) return false;
  if (&s.front() != p || &s.back() != p + 5) return false;
  s.remove_prefix(2);
  if (s.data() != p + 2 || s.size() != 4 || s.front() != 'c') return false;
  s.remove_suffix(1);
  if (s.size() != 3 || s.back() != 'e') return false;
  s.remove_prefix(3);
  if (!s.empty() || s.data() != p + 5) return false;
  SV x("xy"), y("long");
  x.swap(y);
  if (x != "long" || y != "xy") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  bool caught = false;
  try {
    (void)SV("ab").at(2);
  } catch (const std::out_of_range&) {
    caught = true;
  }
  CHECK(caught);
  caught = false;
  try {
    (void)SV().at(0);
  } catch (const std::out_of_range&) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
