// [stacktrace.basic]: basic_stacktrace is a const sequence container of stacktrace_entry.
// current() returns the stacktrace of the current evaluation (or an empty one if obtaining it
// failed); current(skip) drops the first min(n, skip) entries of the stacktrace current()
// would give; current(skip, max_depth) keeps at most max_depth after that. The default
// constructor gives an empty stacktrace; size/empty/begin/end/rbegin/at/operator[] observe
// the frames; at throws out_of_range past the end; == compares element-wise; std::stacktrace
// is basic_stacktrace<allocator<stacktrace_entry>>; hash, to_string and operator<< exist.
#include <stacktrace>
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"

using ST = std::stacktrace;
static_assert(std::is_same_v<ST, std::basic_stacktrace<std::allocator<std::stacktrace_entry>>>);
static_assert(std::is_same_v<ST::value_type, std::stacktrace_entry>);
static_assert(std::is_same_v<ST::const_reference, const std::stacktrace_entry&>);
static_assert(std::is_same_v<ST::iterator, ST::const_iterator>);
static_assert(std::random_access_iterator<ST::const_iterator>);
static_assert(noexcept(ST::current()) && noexcept(ST::current(1)) && noexcept(ST::current(1, 2)));
static_assert(std::is_nothrow_default_constructible_v<ST> && std::is_nothrow_move_constructible_v<ST>);
static_assert(std::is_same_v<decltype(std::to_string(ST())), std::string>);
static_assert(std::is_same_v<decltype(std::to_string(std::stacktrace_entry())), std::string>);

[[gnu::noinline]] ST level2() { return ST::current(); }
[[gnu::noinline]] ST level1() { return level2(); }

int main() {
  ST empty;
  CHECK(empty.empty() && empty.size() == 0 && empty.begin() == empty.end() && empty.rbegin() == empty.rend());
  CHECK(empty.cbegin() == empty.cend() && empty.crbegin() == empty.crend());
  bool threw = false;
  try {
    (void)empty.at(0);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);

  ST t = level1();
  if (!t.empty()) {
    CHECK(static_cast<bool>(t[0]) && t.at(0) == t[0]);
    CHECK(std::distance(t.begin(), t.end()) == static_cast<std::ptrdiff_t>(t.size()));
    CHECK(*t.rbegin() == t[t.size() - 1]);
    CHECK(std::all_of(t.begin(), t.end(), [](const std::stacktrace_entry& e) { return static_cast<bool>(e); }));
    ST copy = t;
    CHECK(copy == t && (copy <=> t) == 0 && std::hash<ST>()(copy) == std::hash<ST>()(t));
    CHECK(std::hash<std::stacktrace_entry>()(t[0]) == std::hash<std::stacktrace_entry>()(copy[0]));
    std::ostringstream os;
    os << t[0] << '\n' << t;
    CHECK(!os.str().empty());
    CHECK(!std::to_string(t).empty());
  }

  // skip and max_depth relative to the full trace of the same evaluation.
  ST full = ST::current();
  ST skipped = ST::current(1);
  ST huge_skip = ST::current(100000);
  ST limited = ST::current(0, 1);
  ST zero = ST::current(0, 0);
  CHECK(huge_skip.empty());
  CHECK(zero.empty());
  CHECK(limited.size() <= 1);
  if (!full.empty()) {
    CHECK(limited.size() == 1);
    CHECK(skipped.size() + 1 == full.size());
  }
  return 0;
}
