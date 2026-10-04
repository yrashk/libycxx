// [string.cons]/20-21: template<class InputIterator> basic_string(begin, end, a) constrained
// on InputIterator qualifying as an input iterator; constructs from [begin, end) as in
// [sequence.reqmts] (/9: each iterator in the range is dereferenced exactly once).
// [string.cons]/22: basic_string(from_range_t, R&&, a) for container-compatible-range<charT>;
// [sequence.reqmts]/12: each iterator in rg is dereferenced exactly once.
// [sequence.reqmts]/69.1: integral types do not qualify as input iterators.
#include <string>
#include <list>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

// The iterator-pair constructor does not take integral arguments.
static_assert(std::is_constructible_v<std::string, int, int>);  // (size_type, charT)
static_assert(std::is_constructible_v<std::string, const char*, const char*>);
static_assert(std::is_constructible_v<std::string, std::from_range_t, std::string_view>);
// container-compatible-range<char> requires the reference to convert to char.
struct NotChar {};
static_assert(!std::is_constructible_v<std::string, std::from_range_t, NotChar (&)[3]>);
static_assert(std::is_constructible_v<std::string, std::from_range_t, int (&)[3]>);

constexpr bool test() {
  char buf[] = "abcdefgh";
  {
    int derefs = 0;
    std::string s(InputIter<char>(buf, &derefs), InputIter<char>(buf + 8, &derefs));
    if (s != "abcdefgh" || derefs != 8) return false;
  }
  {
    int derefs = 0;
    std::string s(ForwardIter<char>(buf, &derefs), ForwardIter<char>(buf + 5, &derefs));
    if (s != "abcde" || derefs != 5) return false;
  }
  {
    std::string s(buf, buf);
    if (!s.empty()) return false;
    std::string t(buf + 1, buf + 3);
    if (t != "bc") return false;
  }
  {
    // element type convertible to char
    int ints[] = {72, 105};
    std::string s(ints, ints + 2);
    if (s != "Hi") return false;
  }
  {
    int derefs = 0;
    std::string s(std::from_range, InputRange<char>{buf, buf + 6, &derefs});
    if (s != "abcdef" || derefs != 6) return false;
  }
  {
    int derefs = 0;
    std::string s(std::from_range, ForwardRange<char>{buf, buf + 3, &derefs});
    if (s != "abc" || derefs != 3) return false;
  }
  {
    std::string s(std::from_range, std::views::iota('a', 'f'));
    if (s != "abcde") return false;
    std::string t(std::from_range, std::string_view("xyz") | std::views::reverse);
    if (t != "zyx") return false;
    char empty[1] = {};
    std::string u(std::from_range, InputRange<char>{empty, empty});
    if (!u.empty()) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::list<char> l = {'x', 'y', 'z'};
  std::string s(l.begin(), l.end());
  CHECK(s == "xyz");
  std::string r(std::from_range, l);
  CHECK(r == "xyz");
  return 0;
}
