// [func.not.fn]/4: not_fn(f) returns "A perfect forwarding call wrapper g with call pattern
// !invoke(fd, call_args...)". [func.require]/5: a call on g "is expression-equivalent to an
// expression e determined from its call pattern" -- so call arguments reach the target with
// their value category and constness, and the result type is that of !invoke(...) (which need
// not be bool). [func.require]/7: copying g copies its state entities memberwise.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Tri {  // a result whose operator! does not yield bool
  int v;
  constexpr Tri operator!() const { return {-v}; }
};
struct Probe {
  constexpr Tri operator()(int&) const { return {1}; }
  constexpr Tri operator()(const int&) const { return {2}; }
  constexpr Tri operator()(int&&) const { return {3}; }
  constexpr Tri operator()(const int&&) const { return {4}; }
};
constexpr bool is_pos(int x) { return x > 0; }
constexpr int as_int(int x) { return x; }
struct Counter {
  int n = 0;
  constexpr bool operator()() { return ++n > 1; }
};
struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  constexpr bool operator()(int x) const { return x == 0; }
};

constexpr bool test() {
  auto n = std::not_fn(Probe{});
  int x = 0;
  const int cx = 0;
  if (n(x).v != -1 || n(cx).v != -2 || n(std::move(x)).v != -3 || n(std::move(cx)).v != -4) return false;
  // double negation
  auto nn = std::not_fn(std::not_fn(is_pos));
  if (!nn(1) || nn(-1)) return false;
  // !int is bool
  auto ni = std::not_fn(as_int);
  if (!ni(0) || ni(5)) return false;
  // a reference_wrapper target calls the referenced object, which is not copied
  Counter c;
  auto nr = std::not_fn(std::ref(c));
  if (!nr() || nr() || c.n != 2) return false;
  // a copy of the wrapper owns an independent copy of the target
  auto n1 = std::not_fn(Counter{});
  if (!n1()) return false;  // n1's counter: 1
  auto n2 = n1;
  if (n2()) return false;   // n2's counter: 2
  if (n1()) return false;   // n1's counter: 2 (unaffected by n2's call)
  // move-only targets are accepted from rvalues
  auto nm = std::not_fn(MoveOnly{});
  if (nm(0) || !nm(1)) return false;
  auto nm2 = std::move(nm);
  if (nm2(0)) return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::not_fn(Probe{})(1)), Tri>);
static_assert(std::is_same_v<decltype(std::not_fn(as_int)(1)), bool>);
static_assert(!std::is_copy_constructible_v<decltype(std::not_fn(MoveOnly{}))>);

int main() {
  CHECK(test());
  return 0;
}
