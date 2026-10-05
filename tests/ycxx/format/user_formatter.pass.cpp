// [format.formatter.spec] Example 1: a program-defined formatter that inherits the parsing of
// a standard specialization; [formatter.requirements]: parse/format with
// basic_format_parse_context and basic_format_context; nested formatting through format_to on
// ctx.out().
// REQUIRES: exceptions
// COUNTERPART: libcxx:utilities/format/format.formatter/format.formatter.spec/formatter.handle.pass.cpp
#include <format>
#include <string>
#include "check.hpp"

enum color { red, green, blue };
const char* color_names[] = {"red", "green", "blue"};
template <>
struct std::formatter<color> : std::formatter<const char*> {
  auto format(color c, format_context& ctx) const { return formatter<const char*>::format(color_names[c], ctx); }
};

struct Point {
  int x, y;
};
template <class CharT>
struct std::formatter<Point, CharT> {
  bool brackets = false;
  constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'b') {
      brackets = true;
      ++it;
    }
    if (it != ctx.end() && *it != '}') throw std::format_error("invalid Point spec");
    return it;
  }
  template <class Ctx>
  auto format(const Point& p, Ctx& ctx) const {
    if (brackets) return std::format_to(ctx.out(), "[{}, {}]", p.x, p.y);
    return std::format_to(ctx.out(), "{} {}", p.x, p.y);
  }
};

int main() {
  CHECK(std::format("{}", red) == "red");
  CHECK(std::format("{:>6}", blue) == "  blue");
  CHECK(std::format("{:.2}", green) == "gr");
  CHECK(std::format("{}|{:b}", Point{1, 2}, Point{3, 4}) == "1 2|[3, 4]");
  bool threw = false;
  try {
    Point p{0, 0};
    (void)std::vformat("{:x}", std::make_format_args(p));
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  static_assert(std::formattable<Point, char>); // its format() accepts any context type
}
