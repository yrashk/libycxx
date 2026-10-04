// [print.fun]/2: print(stream, fmt, args...) is vprint_unicode(stream, fmt.str,
// make_format_args(args...)) (or the _buffered form when locksafe is false, which first
// formats into a string, /8); /5: println appends '\n'. /10: vprint_unicode writes "the
// character representation of formatting arguments provided by args formatted according to
// specifications given in fmt" -- the same characters as vformat (stream is a file, not a
// terminal, /10.2: written unchanged). [ostream.formatted.print]/1-2: print(os, ...) inserts
// vformat(os.getloc(), fmt, args). So for every kind of argument, both locksafe
// (enable_nonlocking_formatter_optimization true, [format.formatter.spec]/3) and not
// (ranges, [format.syn]; program-defined types), the output equals std::format's:
// characters, bool, integers, floating point, every string type, pointers, nullptr, ranges
// (sequence, map, set, string kinds, [format.range.fmtkind]), vector<bool>
// ([vector.bool.fmt]), container adaptors ([container.adaptors.format]), pair / tuple
// ([format.tuple]), chrono durations and time points ([time.format]), thread::id
// ([thread.thread.id]/11-13), filesystem::path ([fs.path.fmtr]) and a program-defined type.
#include <print>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <format>
#include <map>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

struct Money {
  long cents;
};
template <>
struct std::formatter<Money> : std::formatter<long> {
  auto format(Money m, std::format_context& ctx) const {
    auto out = std::formatter<long>::format(m.cents / 100, ctx);
    return std::format_to(out, ".{:02}", m.cents % 100);
  }
};

static std::string slurp(std::FILE* f) {
  std::fflush(f);
  std::rewind(f);
  std::string r;
  for (int c; (c = std::fgetc(f)) != EOF;) r += static_cast<char>(c);
  return r;
}

// print / println to a FILE* and to an ostream give std::format's characters.
template <class... Args>
bool same(std::format_string<Args...> fmt, Args&&... args) {
  const std::string want = std::format(fmt, std::forward<Args>(args)...);
  std::FILE* f = std::tmpfile();
  if (!f) return false;
  std::print(f, fmt, std::forward<Args>(args)...);
  std::println(f, fmt, std::forward<Args>(args)...);
  const std::string got = slurp(f);
  std::fclose(f);
  std::ostringstream os;
  std::print(os, fmt, std::forward<Args>(args)...);
  std::println(os, fmt, std::forward<Args>(args)...);
  return got == want + want + "\n" && os.str() == got;
}

int main() {
  using namespace std::chrono_literals;
  char arr[] = "array";
  char* mp = arr;
  const char* cp = "ptr";
  std::string s = "string";
  std::string_view sv = "view";
  int i = 42;
  void* vp = &i;
  const void* cvp = &i;
  CHECK(same("{} {} {:?} {:d}", 'c', '\n', '\t', 'A'));
  CHECK(same("{} {:s} {:#x} {:5}|", true, false, true, false));
  CHECK(same("{} {} {} {} {}", static_cast<signed char>(-1), static_cast<unsigned char>(255), static_cast<short>(-3),
             65535u, -9223372036854775807LL - 1));
  CHECK(same("{} {:#o} {:+} {:08b} {:X}", 18446744073709551615ULL, 8L, 5UL, 5, 0xbeefLL));
  CHECK(same("{} {} {} {:.3e} {:a} {:g}", 0.1f, 0.1, 0.1L, 1e100, 1.0, 1e-5));
  CHECK(same("{}|{}|{}|{}|{}|{:.2}|{:?}", arr, mp, cp, s, sv, s, "a\"b"));
  CHECK(same("{} {} {} {:P}", vp, cvp, nullptr, vp));
  CHECK(same("{} {:n} {::x}", std::vector<int>{1, 2}, std::vector<int>{3}, std::vector<int>{255}));
  CHECK(same("{} {} {:n}", std::map<int, std::string>{{1, "a"}}, std::set<char>{'x', 'y'}, std::set<int>{4}));
  CHECK(same("{} {:s} {:?s}", std::vector<char>{'h', 'i'}, std::vector<char>{'h', 'i'}, std::vector<char>{'\n'}));
  CHECK(same("{} {::d}", std::vector<bool>{true, false}, std::vector<bool>{true}));
  std::stack<int> st;
  st.push(1);
  st.push(2);
  std::queue<std::string> q;
  q.push("q");
  CHECK(same("{} {}", st, q));
  CHECK(same("{} {:m} {}", std::pair(1, "one"), std::pair('k', 2.5), std::tuple(1, 'c', std::string("s"), nullptr)));
  CHECK(same("{} {:%H:%M} {}", 42ms, 3723s, std::chrono::sys_days(std::chrono::year(2000) / 1 / 2)));
  CHECK(same("{} {:>30}", std::this_thread::get_id(), std::this_thread::get_id()));
  CHECK(same("{} {:?}", std::filesystem::path("a/b"), std::filesystem::path("tab\t")));
  CHECK(same("{} {:>8}", Money{1234}, Money{5}));
  CHECK(same("中{:中^5}\U0001F921", "\U0001F921"));
  CHECK(same("{{}} no arguments"));
  // A few exact results.
  std::ostringstream os;
  std::print(os, "{} {} {:.1f} {} {}", true, 'x', 2.25, std::pair(1, 2), Money{705});
  CHECK(os.str() == "true x 2.2 (1, 2) 7.05");
  return 0;
}
