// [range.utility.conv]: ranges::to with standard containers, including the example of
// [range.utility.conv.general]/2 (a range of ranges converted recursively) and the
// template-template form with deduction.
// COUNTERPART: libcxx:ranges/range.utility/range.utility.conv/to.pass.cpp
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

constexpr bool test() {
  std::string_view str = "the quick brown fox";
  auto words = vw::split(str, ' ') | rg::to<std::vector<std::string>>();
  CHECK(words.size() == 4 && words[0] == "the" && words[3] == "fox");
  auto v = vw::iota(1, 5) | rg::to<std::vector>();
  static_assert(std::is_same_v<decltype(v), std::vector<int>>);
  CHECK(v.size() == 4 && v.back() == 4);
  auto s = str | vw::filter([](char c) { return c != ' '; }) | rg::to<std::string>();
  CHECK(s == "thequickbrownfox");
  auto nested = vw::iota(1, 4) | vw::transform([](int n) { return vw::iota(0, n); }) | rg::to<std::vector<std::vector<int>>>();
  CHECK(nested.size() == 3 && nested[2].size() == 3 && nested[2][2] == 2);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
