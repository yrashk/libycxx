// [format.tuple]: pair and tuple format as (e1, e2, ...) with elements in debug format (/7);
// n removes the brackets; m (only for two elements) gives "k: v"; width, fill and alignment
// apply to the whole tuple; set_separator/set_brackets.
// COUNTERPART: libcxx:utilities/format/format.tuple/format.pass.cpp
#include <format>
#include <string>
#include <tuple>
#include <utility>
#include "check.hpp"

int main() {
  CHECK(std::format("{}", std::pair(1, 'a')) == "(1, 'a')");
  CHECK(std::format("{}", std::tuple(1, "s", 2.5)) == "(1, \"s\", 2.5)");
  CHECK(std::format("{}", std::tuple<>()) == "()");
  CHECK(std::format("{}", std::tuple(7)) == "(7)");
  CHECK(std::format("{:n}", std::pair(1, 2)) == "1, 2");
  CHECK(std::format("{:m}", std::pair(1, 2)) == "1: 2");
  CHECK(std::format("{:m}", std::tuple(std::string("k"), 'v')) == "\"k\": 'v'");
  CHECK(std::format("{:*^10}", std::pair(1, 2)) == "**(1, 2)**");
  CHECK(std::format("{:>8n}", std::pair(1, 2)) == "    1, 2");
  CHECK(std::format("{}", std::pair(std::pair(1, 2), std::tuple('x'))) == "((1, 2), ('x'))");
  CHECK(std::format(L"{}", std::pair(1, L'w')) == L"(1, 'w')");
  std::formatter<std::pair<int, int>, char> f;
  f.set_separator(" - ");
  f.set_brackets("<", ">");
  static_assert(noexcept(f.set_separator("")) && noexcept(f.set_brackets("", "")));
  static_assert(std::formattable<std::tuple<int, std::string>, char>);
  struct NoFmt {};
  static_assert(!std::formattable<std::pair<int, NoFmt>, char>);
}
