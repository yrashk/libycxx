// [format.string.std]/3-/4, /9-/11 and Table 104: fill and alignment (default left for
// strings, chars and bool, right for arithmetic types and pointers), centring with
// floor(n/2) fill characters before and ceil(n/2) after, no padding when the field is already
// wide enough (dynamic width: dynamic_width_precision), and the examples of
// [format.string.std]/4.
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

int main() {
  CHECK(std::format("{:6}", 42) == "    42");
  CHECK(std::format("{:6}", 'x') == "x     ");
  CHECK(std::format("{:6}", true) == "true  ");
  CHECK(std::format("{:*<6.3}", "123456") == "123***");
  CHECK(std::format("{:*<}", "12") == "12");
  CHECK(std::format("{:*<6}", "12345678") == "12345678");
  CHECK(std::format("{:^7}", "ab") == "  ab   ");
  CHECK(std::format("{:-^7}", "abc") == "--abc--");
  CHECK(std::format("{:>>4}", 1) == ">>>1"); // '>' as fill
  CHECK(std::format("{:<<4}", 1) == "1<<<");
  CHECK(std::format("{:^^4}", 1) == "^1^^");
  CHECK(std::format("{:0>4}", 7) == "0007"); // '0' as a fill character
  CHECK(std::format("{:8}", std::string_view("sv")) == "sv      ");
  CHECK(std::format("{:>8}", std::string("str")) == "     str");
  // Width larger than the output by one: centring puts the extra fill after.
  CHECK(std::format("{:^4}", "abc") == "abc ");
  CHECK(std::format("{:^2}", 'z') == "z ");
}
