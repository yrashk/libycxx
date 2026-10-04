// [format.range.fmtdef]: formatter<R, charT> for an input_range R with format_kind<R> not
// disabled; [format.range.fmtkind]: format_kind is range_format::sequence for a generator
// (its value type is not a pair/tuple of size 2 nor the range itself);
// [format.arg]/5 with make_format_args(Args&...): a non-const lvalue generator (not
// const-iterable) is formattable through its non-const formatter ([format.range.fmtdef]/2:
// maybe-const-r is R when input_range<const R> is false). [format.range.formatter]/9: with no
// range-underlying-spec the underlying formatter is set to debug format, so strings are quoted
// and escaped; /11: the n option omits the brackets; range-type s turns a range of char into a
// string. Same for an input-only view (views::as_input) and a non-const-iterable filter view.
#include <generator>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>
#include "check.hpp"

std::generator<int> ints() {
  co_yield 1;
  co_yield 2;
  co_yield 3;
}
std::generator<const std::string&> strs() {
  co_yield "a";
  co_yield "b\n";
}
std::generator<char> chars() {
  co_yield 'h';
  co_yield 'i';
}
std::generator<std::pair<int, std::string_view>> pairs() {
  co_yield {1, "x"};
  co_yield {2, "y"};
}

int main() {
  auto g = ints();
  CHECK(std::format("{}", g) == "[1, 2, 3]");
  auto h = strs();
  CHECK(std::format("{}", h) == "[\"a\", \"b\\n\"]");
  auto c = chars();
  CHECK(std::format("{:s}", c) == "hi");
  auto c2 = chars();
  CHECK(std::format("{}", c2) == "['h', 'i']");
  auto n = ints();
  CHECK(std::format("{:n:03}", n) == "001, 002, 003");
  auto p = pairs();
  CHECK(std::format("{:m}", p) == "{1: \"x\", 2: \"y\"}");
  std::vector<int> v{1, 2, 3, 4};
  auto f = v | std::views::filter([](int x) { return x % 2 == 0; });
  CHECK(std::format("{:n}", f) == "2, 4");
  auto in = v | std::views::as_input;
  CHECK(std::format("{}", in) == "[1, 2, 3, 4]");
  return 0;
}
