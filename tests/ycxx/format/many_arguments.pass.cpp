// Formatting with hundreds of arguments of mixed types: the draft sets no limit on the number of
// formatting arguments.
//   [format.arg.store]/3: make_format_args stores one basic_format_arg per argument
//     (array<basic_format_arg<Context>, sizeof...(Args)>); [format.args]/5 get(i): "Returns:
//     i < size_ ? data_[i] : basic_format_arg<Context>()" (no argument: !arg is true,
//     [format.arg]/... operator bool).
//   [format.string.general]/4-6: automatic and manual argument indexing; [format.string.std]/
//     width and precision taken from an argument ("{0:{250}}"), with the argument index beyond
//     any small packed representation.
//   [format.functions] format, vformat, format_to_n, formatted_size; wide strings likewise.
// 300 arguments cycling through int, double, string_view, const char*, bool, char, long long,
// unsigned, a pointer and a user type with its own formatter.
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include "check.hpp"

struct Point {
  int x, y;
};
template<class CharT> struct std::formatter<Point, CharT> {
  constexpr auto parse(std::basic_format_parse_context<CharT>& pc) { return pc.begin(); }
  template<class Ctx> auto format(const Point& p, Ctx& ctx) const {
    if constexpr (std::is_same_v<CharT, char>)
      return std::format_to(ctx.out(), "({},{})", p.x, p.y);
    else
      return std::format_to(ctx.out(), L"({},{})", p.x, p.y);
  }
};

constexpr int N = 300;
static int ints[N];
static const char* const words[] = {"alpha", "beta", "gamma"};

template<std::size_t I> auto arg() {
  if constexpr (I % 10 == 0) return static_cast<int>(I);
  else if constexpr (I % 10 == 1) return static_cast<double>(I) + 0.5;
  else if constexpr (I % 10 == 2) return std::string_view(words[I % 3]);
  else if constexpr (I % 10 == 3) return words[(I + 1) % 3];
  else if constexpr (I % 10 == 4) return I % 20 == 4;
  else if constexpr (I % 10 == 5) return static_cast<char>('a' + I % 26);
  else if constexpr (I % 10 == 6) return static_cast<long long>(I) * -1000000000LL;
  else if constexpr (I % 10 == 7) return static_cast<unsigned>(I);
  else if constexpr (I % 10 == 8) return static_cast<const void*>(&ints[I]);
  else return Point{static_cast<int>(I), -static_cast<int>(I)};
}

template<std::size_t I> std::string expected_text() {
  if constexpr (I % 10 == 8) return std::format("{}", arg<I>());
  else if constexpr (I % 10 == 9) return "(" + std::to_string(I) + "," + std::to_string(-static_cast<int>(I)) + ")";
  else if constexpr (I % 10 == 4) return I % 20 == 4 ? "true" : "false";
  else if constexpr (I % 10 == 5) return std::string(1, static_cast<char>('a' + I % 26));
  else if constexpr (I % 10 == 1) return std::to_string(I) + ".5";
  else if constexpr (I % 10 == 2) return words[I % 3];
  else if constexpr (I % 10 == 3) return words[(I + 1) % 3];
  else if constexpr (I % 10 == 6) return std::to_string(static_cast<long long>(I) * -1000000000LL);
  else return std::to_string(I);
}

template<std::size_t... I> void run(std::index_sequence<I...>) {
  std::string automatic, manual_reversed, want, want_reversed;
  for (int i = 0; i < N; ++i) {
    automatic += "{}|";
    manual_reversed += "{" + std::to_string(N - 1 - i) + "}|";
  }
  const std::string parts[] = {expected_text<I>()...};
  for (int i = 0; i < N; ++i) {
    want += parts[i] + "|";
    want_reversed += parts[N - 1 - i] + "|";
  }
  auto values = std::make_tuple(arg<I>()...);
  auto args = std::apply([](auto&... v) { return std::make_format_args(v...); }, values);
  CHECK(std::vformat(automatic, args) == want);
  CHECK(std::vformat(manual_reversed, args) == want_reversed);
  CHECK(std::format(std::runtime_format(automatic), arg<I>()...) == want);
  CHECK(std::formatted_size(std::runtime_format(manual_reversed), arg<I>()...) == want_reversed.size());
  char buf[64];
  const auto r = std::format_to_n(buf, sizeof buf, std::runtime_format(automatic), arg<I>()...);
  CHECK(r.size == static_cast<std::ptrdiff_t>(want.size()));
  CHECK(std::string_view(buf, sizeof buf) == std::string_view(want).substr(0, sizeof buf));

  // Width and precision from far arguments (indices 250 and 290 hold ints 250 and 290).
  CHECK(std::vformat("[{0:>{250}}]", args).size() == 252);
  CHECK(std::vformat("[{1:.{290}f}]", args) == "[" + std::format("{:.290f}", 1.5) + "]");
  CHECK(std::vformat("{299}{0}{299}", args) == parts[N - 1] + "0" + parts[N - 1]);

  std::format_args fa = args;
  CHECK(static_cast<bool>(fa.get(N - 1)));
  CHECK(!fa.get(N));
  CHECK(!fa.get(N + 1000));

  // An index past the last argument is an error.
  bool threw = false;
  try {
    (void)std::vformat("{300}", args);
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);

  // Wide: the same with the integers only (no conversion of narrow strings).
  std::wstring wauto, wwant;
  for (int i = 0; i < N; ++i) {
    wauto += L"{}";
    wwant += std::to_wstring(i * 7);
  }
  CHECK(std::format(std::runtime_format(wauto), (static_cast<int>(I) * 7)...) == wwant);
}

int main() {
  run(std::make_index_sequence<N>{});
  return 0;
}
