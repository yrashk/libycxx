// [format.range.fmtkind]/1-2: format_kind<R> is range_format::disabled when R's reference
// type (cvref removed) is R itself, map when R::key_type and R::mapped_type exist and the
// reference is a pair or a 2-tuple, set when only R::key_type exists, sequence otherwise --
// for the standard containers and for program-defined ranges; /3: a program may specialize
// format_kind for its own ranges ([format.range.fmtdef]), e.g. as set (formatted with braces,
// [format.range.fmtset]/1), string or debug_string (the elements, which must be the
// character type, formatted as one string, [format.range.fmtstr]/2-4). formattable is false
// for a disabled range ([format.range.fmtdef]: the formatter requires format_kind<R> !=
// range_format::disabled).
#include <cstddef>
#include <deque>
#include <flat_map>
#include <flat_set>
#include <format>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "check.hpp"

using std::range_format;

// a range of itself, like filesystem::path
struct SelfRange {
  const SelfRange* begin() const { return this; }
  const SelfRange* end() const { return this; }
};

// key_type + mapped_type, but the reference is not pair-like: a set
struct KeyedInts {
  using key_type = int;
  using mapped_type = int;
  std::vector<int> v;
  auto begin() const { return v.begin(); }
  auto end() const { return v.end(); }
};
// key_type + mapped_type with 2-tuples: a map
struct TupleMap {
  using key_type = int;
  using mapped_type = char;
  std::vector<std::tuple<int, char>> v;
  auto begin() const { return v.begin(); }
  auto end() const { return v.end(); }
};
// program-defined ranges with specialized format_kind
struct MySet {
  std::vector<int> v;
  auto begin() const { return v.begin(); }
  auto end() const { return v.end(); }
};
struct MyChars {
  std::vector<char> v;
  auto begin() const { return v.begin(); }
  auto end() const { return v.end(); }
};
struct MyDebugChars {
  std::vector<char> v;
  auto begin() const { return v.begin(); }
  auto end() const { return v.end(); }
};
template <>
inline constexpr range_format std::format_kind<MySet> = range_format::set;
template <>
inline constexpr range_format std::format_kind<MyChars> = range_format::string;
template <>
inline constexpr range_format std::format_kind<MyDebugChars> = range_format::debug_string;

static_assert(std::format_kind<std::vector<int>> == range_format::sequence);
static_assert(std::format_kind<std::deque<std::string>> == range_format::sequence);
static_assert(std::format_kind<std::set<int>> == range_format::set);
static_assert(std::format_kind<std::multiset<int>> == range_format::set);
static_assert(std::format_kind<std::unordered_set<int>> == range_format::set);
static_assert(std::format_kind<std::flat_set<int>> == range_format::set);
static_assert(std::format_kind<std::map<int, int>> == range_format::map);
static_assert(std::format_kind<std::unordered_multimap<int, int>> == range_format::map);
static_assert(std::format_kind<std::flat_map<int, int>> == range_format::map);  // reference pair<const int&, int&>
static_assert(std::format_kind<SelfRange> == range_format::disabled);
static_assert(!std::formattable<SelfRange, char>);
static_assert(std::format_kind<KeyedInts> == range_format::set);
static_assert(std::format_kind<TupleMap> == range_format::map);
static_assert(std::format_kind<MySet> == range_format::set);

int main() {
  CHECK(std::format("{}", KeyedInts{{3, 1}}) == "{3, 1}");
  CHECK(std::format("{}", TupleMap{{{1, 'a'}, {2, 'b'}}}) == "{1: 'a', 2: 'b'}");
  CHECK(std::format("{}", MySet{{1, 2}}) == "{1, 2}");
  CHECK(std::format("{:n}", MySet{{1, 2}}) == "1, 2");
  CHECK(std::format("{}", MyChars{{'h', 'i', '\n'}}) == "hi\n");
  CHECK(std::format("{:>4}", MyChars{{'h', 'i'}}) == "  hi");
  CHECK(std::format("{}", MyDebugChars{{'h', 'i', '\n'}}) == "\"hi\\n\"");
  CHECK(std::format("{}", std::flat_map<int, char>{{1, 'x'}}) == "{1: 'x'}");
  CHECK(std::format("{}", std::flat_set<int>{2, 1}) == "{1, 2}");
  return 0;
}
