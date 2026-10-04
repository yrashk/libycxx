// string_view / string / span together with <ranges> and <format>:
// [string.view.cons]/12-15: basic_string_view(R&& r) (explicit) from a contiguous, sized range
// of charT that is not convertible to const charT* -- the subranges produced by views::split
// of a string_view ([range.split.view]: value_type subrange<iterator_t<V>>, contiguous for a
// string_view base) and views::take / views::drop of a std::string. [range.split.iterator]:
// "a,,b," splits into "a", "", "b", "" (a trailing empty piece), "" into nothing, and a
// multi-character pattern is matched as a whole.
// [range.utility.conv.to]: ranges::to<std::string>(views::join(...)) and views::join_with.
// [span.cons]/15-16: span<T>(R&&) requires contiguous_range and sized_range: not
// constructible from a filter view, a list, or an iota view; constructible from take of a
// vector. [format.range.formatter]: a range of char elements formats as a sequence of
// debug-formatted chars ("['a', 'b']"); range-type s formats it as a string, ?s as an escaped
// string; "{::s}" applies s to the inner ranges; the n option drops the brackets; fill/align
// and width apply to the whole output.
#include <string_view>
#include <format>
#include <list>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>
#include "check.hpp"

using namespace std::string_view_literals;
namespace rv = std::views;

static std::vector<std::string_view> pieces(std::string_view s, std::string_view pat) {
  std::vector<std::string_view> out;
  for (auto sub : s | rv::split(pat)) out.emplace_back(sub);  // explicit range constructor
  return out;
}

using Sub = std::ranges::range_value_t<decltype("a"sv | rv::split(','))>;
static_assert(std::ranges::contiguous_range<Sub> && std::ranges::sized_range<Sub>);
static_assert(std::is_constructible_v<std::string_view, Sub>);
static_assert(!std::is_convertible_v<Sub, std::string_view>);  // explicit
static_assert(!std::is_constructible_v<std::string_view, std::list<char>&>);
static_assert(!std::is_constructible_v<std::string_view, std::vector<wchar_t>&>);

using Filtered = decltype(std::declval<std::vector<int>&>() | rv::filter([](int) { return true; }));
static_assert(!std::is_constructible_v<std::span<int>, Filtered>);
static_assert(!std::is_constructible_v<std::span<int>, std::list<int>&>);
static_assert(!std::is_constructible_v<std::span<const int>, std::ranges::iota_view<int, int>>);
static_assert(std::is_constructible_v<std::span<int>, decltype(std::declval<std::vector<int>&>() | rv::take(2))>);
static_assert(std::is_constructible_v<std::span<const int>, decltype(std::declval<const std::vector<int>&>() | rv::drop(1))>);

int main() {
  auto v = pieces("a,,b,", ",");
  CHECK(v.size() == 4 && v[0] == "a" && v[1].empty() && v[2] == "b" && v[3].empty());
  CHECK(pieces("", ",").empty());
  auto w = pieces("x--y-z--", "--");
  CHECK(w.size() == 3 && w[0] == "x" && w[1] == "y-z" && w[2].empty());
  auto one = pieces("abc", ",");
  CHECK(one.size() == 1 && one[0] == "abc");
  // The pieces point into the original storage.
  std::string_view text = "key=value";
  auto kv = pieces(text, "=");
  CHECK(kv[1].data() == text.data() + 4);

  std::string s = "hello world";
  std::string_view head(s | rv::take(5));
  std::string_view tail(s | rv::drop(6));
  CHECK(head == "hello" && head.data() == s.data() && tail == "world");
  std::string_view all(s | rv::take(100));
  CHECK(all == s);

  std::vector<std::string> words{"ab", "", "cd"};
  CHECK(std::ranges::to<std::string>(words | rv::join) == "abcd");
  CHECK(std::ranges::to<std::string>(words | rv::join_with(", "sv)) == "ab, , cd");
  CHECK(std::ranges::to<std::string>("a b c"sv | rv::split(' ') | rv::join) == "abc");
  CHECK(std::ranges::to<std::string>("a b c"sv | rv::split(' ') | rv::join_with('+')) == "a+b+c");
  auto vs = std::ranges::to<std::vector<std::string>>("x,y"sv | rv::split(','));
  CHECK(vs.size() == 2 && vs[0] == "x" && vs[1] == "y");

  // span over a take of a vector.
  std::vector<int> nums{1, 2, 3, 4};
  std::span<int> sp(nums | rv::take(3));
  CHECK(sp.size() == 3 && sp.data() == nums.data());

  // format.
  CHECK(std::format("{}", "ab c"sv | rv::split(' ')) == "[['a', 'b'], ['c']]");
  CHECK(std::format("{::s}", "ab c"sv | rv::split(' ')) == "[ab, c]");
  CHECK(std::format("{::?s}", "ab c\t"sv | rv::split(' ')) == "[\"ab\", \"c\\t\"]");
  CHECK(std::format("{:s}", s | rv::take(5)) == "hello");
  CHECK(std::format("{:?s}", s | rv::take(5)) == "\"hello\"");
  CHECK(std::format("{}", s | rv::take(2)) == "['h', 'e']");
  CHECK(std::format("{}", std::span<const int>(nums)) == "[1, 2, 3, 4]");
  CHECK(std::format("{:n:02}", std::span<const int>(nums).first(2)) == "01, 02");
  CHECK(std::format("{:*>12}", std::span<const int>(nums).first(2)) == "******[1, 2]");
  CHECK(std::format("{:*^8s}", s | rv::take(2)) == "***he***");
  CHECK(std::format("{}", std::vector<std::string_view>{"a", "b\n"}) == "[\"a\", \"b\\n\"]");
  CHECK(std::format("{:n}", words | rv::transform([](const std::string& x) -> std::string_view { return x; })) ==
        "\"ab\", \"\", \"cd\"");
  return 0;
}
