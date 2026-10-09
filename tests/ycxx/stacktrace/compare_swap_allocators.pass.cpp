// [stacktrace.basic.cmp]/1-2: == compares the entries, also between stacktraces with different
// allocators; <=> is a strong_ordering that compares the sizes first and, only for equal sizes,
// the entries lexicographically (so a deeper stacktrace is greater whatever its entries).
// [stacktrace.basic.mod]/1: swap is noexcept when the allocator propagates on swap or is always
// equal; [res.on.exception.handling]/5 permits strengthening when the condition is false.
// [stacktrace.basic.nonmem]/1: the non-member swap has the member's exception
// specification; /4-6: to_string and << for a stacktrace with another allocator.
// [stacktrace.basic.hash]: hash is enabled for every allocator.
// [stacktrace.basic.cons]: current(alloc) uses the allocator; get_allocator() returns it.
#include <stacktrace>
#include <algorithm>
#include <compare>
#include <functional>
#include <sstream>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "test_allocators.hpp"

using E = std::stacktrace_entry;
using A = IdAlloc<E>;
using AS = std::basic_stacktrace<A>;
using SwapA = IdAlloc<E, false, false, true>;
using SS = std::basic_stacktrace<SwapA>;

static_assert(noexcept(std::declval<std::stacktrace&>().swap(std::declval<std::stacktrace&>())));
static_assert(noexcept(std::declval<SS&>().swap(std::declval<SS&>())));
static_assert(noexcept(swap(std::declval<SS&>(), std::declval<SS&>())));
static_assert(std::is_same_v<decltype(std::stacktrace() <=> AS()), std::strong_ordering>);
static_assert(noexcept(std::stacktrace() == AS()) && noexcept(std::stacktrace() <=> AS()));
static_assert(std::is_default_constructible_v<std::hash<AS>>);

[[gnu::noinline]] AS deeper(int n, const A& a) {
  if (n == 0) return AS::current(a);
  AS r = deeper(n - 1, a);
  asm volatile("" ::: "memory"); // not a tail call
  return r;
}

int main() {
  A a7(7);
  AS here = AS::current(a7);
  CHECK(here.get_allocator().id == 7);
  if (here.empty()) return 0; // no stacktrace available: nothing more to compare

  AS deep = deeper(5, a7);
  // Each capture can independently fail and the frames approximate the evaluation.
  // Comparison follows the observed sizes/entries, without assuming deeper capture success.
  if (deep.size() != here.size()) {
    CHECK((deep <=> here) == (deep.size() <=> here.size()));
    CHECK((here <=> deep) == (here.size() <=> deep.size()));
    CHECK(deep != here);
  } else {
    CHECK((deep <=> here) == std::lexicographical_compare_three_way(
        deep.begin(), deep.end(), here.begin(), here.end()));
  }
  // Equal sizes: entry-wise, across allocators.
  AS copy(here, A(9));
  CHECK(copy.get_allocator().id == 9);
  std::stacktrace plain = std::stacktrace::current();
  if (!copy.empty()) CHECK(copy == here && (copy <=> here) == std::strong_ordering::equal);
  bool cross = (plain == here) == std::equal(plain.begin(), plain.end(), here.begin(), here.end());
  CHECK(cross);
  if (plain.size() == here.size()) {
    auto expect = std::lexicographical_compare_three_way(plain.begin(), plain.end(), here.begin(), here.end());
    CHECK((plain <=> here) == expect);
  } else {
    CHECK((plain <=> here) == (plain.size() <=> here.size()));
  }
  // Hash agrees with ==.
  if (copy == here) CHECK(std::hash<AS>()(copy) == std::hash<AS>()(here));
  // Swap.
  AS empty(a7);
  AS full = here;
  if (full.empty()) return 0;  // permitted allocation failure, [stacktrace.basic.cons]/10
  full.swap(empty);
  CHECK(full.empty() && empty == here);
  swap(full, empty);
  CHECK(full == here && empty.empty());
  // to_string and <<.
  std::ostringstream os;
  os << here;
  CHECK(os.str() == std::to_string(here) && !os.str().empty());
  return 0;
}
