// [vector.bool.fmt], [format.formatter.spec]/2: <vector> provides formatter<vector<bool>::reference>
// and the arithmetic, string and pointer formatters; the formatting functions come with <format>,
// included afterwards. A function template defined before <format> that uses the formatters
// works once instantiated after it ([temp.point]); vector<bool>::reference formats as bool
// ([vector.bool.fmt]/1-2), so the bool presentations apply.
#include <vector>
#include <concepts>

static_assert(std::semiregular<std::formatter<std::vector<bool>::reference, char>>);
static_assert(std::semiregular<std::formatter<int, char>>);

// A program-defined formatter written before <format>, delegating to the one of <vector>.
struct Flag {
  std::vector<bool>::reference r;
};
template <>
struct std::formatter<Flag> {
  std::formatter<std::vector<bool>::reference> f;
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& pc) {
    return f.parse(pc);
  }
  template <class FormatContext>
  typename FormatContext::iterator format(const Flag& x, FormatContext& ctx) const {
    return f.format(x.r, ctx);
  }
};

#include <format>
#include "check.hpp"

int main() {
  std::vector<bool> v{true, false, true};
  CHECK(std::format("{} {}", v[0], v[1]) == "true false");
  CHECK(std::format("{:d}|{:>6}|{:#x}", v[0], v[1], v[2]) == "1| false|0x1");
  CHECK(std::format(L"{:s}", v[1]) == L"false");
  CHECK(std::format("{}", v) == "[true, false, true]");
  CHECK(std::format("{:?}", 'a') == "'a'");
  CHECK(std::format("[{:*<6}]", Flag{v[2]}) == "[true**]");
  CHECK(std::format("{:d}", Flag{v[1]}) == "0");
}
