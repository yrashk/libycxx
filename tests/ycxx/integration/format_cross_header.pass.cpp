// std::format over values whose formatters come from many headers, nested inside each other:
// the range, tuple and map formatters ([format.range.formatter], [format.range.fmtmap],
// [format.tuple]) composing <filesystem>, <chrono>, <thread>, <vector> (vector<bool>), the
// container adaptors and views.
//   [format.range.formatter]/9: parse calls underlying_.set_debug_format() when the range-type
//     is neither s nor ?s, set_debug_format is valid and there is no range-underlying-spec; so
//     strings, chars and paths inside ranges are escaped and quoted
//     ([format.string.escaped]), and "n" removes only the brackets.
//   [format.tuple]/7: likewise for every element of a pair or tuple.
//   [fs.path.fmtr.funcs]: path has set_debug_format; "{}" writes native(), "{:?}" escaped,
//     "{:g}" generic.
//   [time.format]: durations "{}" as operator<< ("1s", "250ms"); sys_days as "%F".
//   [thread.thread.id]: formatter<thread::id> writes what operator<< writes.
//   [vector.bool.fmt]: vector<bool>::reference formats as bool.
//   [container.adaptors.format]: stack/queue/priority_queue format their underlying container
//     (for priority_queue that is the heap order, here checked against the container itself).
//   [format.range.fmtkind]/[optional.syn]: format_kind<optional<T>> is range_format::disabled,
//     so optional<int> is not formattable (although it is a range).
#include <chrono>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <queue>
#include <ranges>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

int main() {
  std::vector<fs::path> paths{"a/b", "c d", "tab\there"};
  CHECK(std::format("{}", paths) == R"(["a/b", "c d", "tab\there"])");
  CHECK(std::format("{:n}", paths) == R"("a/b", "c d", "tab\there")");
  CHECK(std::format("{::}", paths) == "[a/b, c d, tab\there]");  // empty underlying spec: no debug
  CHECK(std::format("{}|{:?}|{:>6}", fs::path("x/y"), fs::path("q\"r"), fs::path("ab")) == "x/y|\"q\\\"r\"|    ab");
  std::map<std::string, fs::path> mp{{"home", "/h"}, {"tmp", "/t"}};
  CHECK(std::format("{}", mp) == R"({"home": "/h", "tmp": "/t"})");
  CHECK(std::format("{}", std::make_tuple(fs::path("x"), std::string_view("y"), 'z', 1.5)) == R"(("x", "y", 'z', 1.5))");

  std::vector<std::chrono::milliseconds> ds{1000ms, 250ms};
  CHECK(std::format("{}", ds) == "[1000ms, 250ms]");
  std::map<std::string, std::chrono::sys_days> days{{"launch", std::chrono::sys_days(std::chrono::year(2026) / 10 / 4)}};
  CHECK(std::format("{}", days) == R"({"launch": 2026-10-04})");
  CHECK(std::format("{::%Y}", std::vector{std::chrono::sys_days(std::chrono::year(1999) / 1 / 1)}) == "[1999]");

  std::ostringstream os;
  os << std::this_thread::get_id();
  CHECK(std::format("{}", std::this_thread::get_id()) == os.str());
  CHECK(std::format("{}", std::vector{std::this_thread::get_id()}) == "[" + os.str() + "]");

  std::vector<bool> vb{true, false, true};
  CHECK(std::format("{}", vb) == "[true, false, true]");
  CHECK(std::format("{::d}", vb) == "[1, 0, 1]");
  CHECK(std::format("{}", std::vector<std::vector<bool>>{{true}, {}}) == "[[true], []]");

  std::vector<char> chars{'a', '\n', 'b'};
  CHECK(std::format("{}", chars) == R"(['a', '\n', 'b'])");
  CHECK(std::format("{:s}", std::vector<char>{'a', 'b'}) == "ab");
  CHECK(std::format("{:?s}", chars) == R"("a\nb")");

  std::vector<int> v1{1, 2, 3};
  std::vector<std::string> v2{"x", "y"};
  CHECK(std::format("{}", std::views::zip(v1, v2)) == R"([(1, "x"), (2, "y")])");
  auto odd = v1 | std::views::filter([](int x) { return x % 2 == 1; });  // not const-iterable
  CHECK(std::format("{}", odd) == "[1, 3]");
  CHECK(std::format("{}", std::vector<std::pair<int, std::vector<std::string>>>{{1, {"a"}}, {2, {}}}) ==
        R"([(1, ["a"]), (2, [])])");

  std::stack<int> st;
  st.push(1);
  st.push(2);
  std::queue<std::string> q;
  q.push("p");
  q.push("q");
  CHECK(std::format("{}", st) == "[1, 2]");
  CHECK(std::format("{}", q) == R"(["p", "q"])");
  struct Peek : std::priority_queue<int> {
    const std::vector<int>& container() const { return c; }
  } pq;
  for (int i : {3, 1, 4, 1, 5}) pq.push(i);
  CHECK(std::format("{}", static_cast<const std::priority_queue<int>&>(pq)) == std::format("{}", pq.container()));

  static_assert(!std::formattable<std::optional<int>, char>);
  static_assert(std::format_kind<std::optional<int>> == std::range_format::disabled);
  static_assert(std::formattable<std::vector<fs::path>, char>);
  return 0;
}
