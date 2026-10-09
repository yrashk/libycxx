// [stacktrace.basic]: basic_stacktrace is a const sequence container of stacktrace_entry.
// current() returns the stacktrace of the current evaluation (or an empty one if obtaining it
// failed); current(skip) drops the first min(n, skip) entries of the stacktrace current()
// would give; current(skip, max_depth) keeps at most max_depth after that. The default
// constructor gives an empty stacktrace; size/empty/begin/end/rbegin/at/operator[] observe
// the frames; at throws out_of_range past the end; == compares element-wise; std::stacktrace
// is basic_stacktrace<allocator<stacktrace_entry>>; hash, to_string and operator<< exist.
// REQUIRES: exceptions
#include <stacktrace>
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"
#include "test_allocators.hpp"

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

  // Each current() call captures a different evaluation and may fail independently.
  // Check each call's cap; no cross-call depth or success relation is specified.
  const auto max_skip = std::numeric_limits<ST::size_type>::max();
  CHECK(ST::current(max_skip).empty());
  for (ST::size_type skip : {ST::size_type(0), ST::size_type(1)}) {
    for (ST::size_type depth : {ST::size_type(0), ST::size_type(1), ST::size_type(3)}) {
      ST limited = ST::current(skip, depth);
      CHECK(limited.size() <= depth);
      CHECK(limited.empty() == (limited.size() == 0));
    }
  }
  CHECK(ST::current(max_skip, 0).empty());  // no skip + max_depth overflow

  // Deterministic allocation failure must be represented by an empty capture.
  using FailingST = std::basic_stacktrace<CountingAlloc<std::stacktrace_entry>>;
  alloc_counters.fail_after = 0;
  CHECK(FailingST::current().empty());
  CHECK(FailingST::current(1).empty());
  CHECK(FailingST::current(0, 3).empty());
  alloc_counters.fail_after = -1;
  CHECK(alloc_counters.outstanding == 0);
  return 0;
}
