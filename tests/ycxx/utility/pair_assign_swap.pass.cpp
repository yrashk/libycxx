// [pairs.pair]/20-22: copy assignment, deleted unless both members are copy-assignable.
// /26-28, /39-41: converting assignment from pair<U1, U2>. /32-35: move assignment with
// noexcept(is_nothrow_move_assignable_v<T1> && is_nothrow_move_assignable_v<T2>). /42-44:
// assignment from pair-like types. /23-25, /36-38, /29-31, /45-50: const-qualified assignments
// assign through reference members. /51-54: swap and the const swap, noexcept per
// is_nothrow_swappable_v. [pairs.spec]/4-5: non-member swap, constrained on is_swappable_v.
#include <utility>
#include <array>
#include <tuple>
#include <type_traits>
#include "check.hpp"

struct ThrowingMoveAssign {
  ThrowingMoveAssign& operator=(ThrowingMoveAssign&&) { return *this; }
  ThrowingMoveAssign& operator=(const ThrowingMoveAssign&) = default;
};
struct NoCopyAssign {
  NoCopyAssign& operator=(const NoCopyAssign&) = delete;
};
struct NotSwappable {
  NotSwappable& operator=(NotSwappable&&) = delete;
};

static_assert(std::is_nothrow_move_assignable_v<std::pair<int, double>>);
static_assert(std::is_move_assignable_v<std::pair<int, ThrowingMoveAssign>>);
static_assert(!std::is_nothrow_move_assignable_v<std::pair<int, ThrowingMoveAssign>>);
static_assert(!std::is_copy_assignable_v<std::pair<int, NoCopyAssign>>);
static_assert(std::is_copy_assignable_v<std::pair<int&, int>>);  // assigns through the reference
static_assert(!std::is_assignable_v<std::pair<int, int>&, std::pair<int*, int>>);
static_assert(std::is_assignable_v<std::pair<long, double>&, const std::pair<int, float>&>);
static_assert(std::is_assignable_v<std::pair<long, long>&, std::array<int, 2>>);
static_assert(!std::is_assignable_v<std::pair<long, long>&, std::array<int, 3>>);
// const assignment only through reference members
static_assert(std::is_assignable_v<const std::pair<int&, int&>&, const std::pair<int, int>&>);
static_assert(!std::is_assignable_v<const std::pair<int, int>&, const std::pair<int, int>&>);
// swap
static_assert(std::is_nothrow_swappable_v<std::pair<int, double>>);
static_assert(!std::is_swappable_v<std::pair<int, NotSwappable>>);
static_assert(std::is_swappable_v<const std::pair<int&, int&>>);
static_assert(!std::is_swappable_v<const std::pair<int, int>>);

constexpr bool test() {
  std::pair<int, double> a(1, 2.0), b(3, 4.0);
  std::pair<int, double>& r = (a = b);
  if (&r != &a || a.first != 3 || a.second != 4.0) return false;
  std::pair<short, float> s(5, 6.0f);
  a = s;
  if (a.first != 5 || a.second != 6.0) return false;
  a = std::pair<int, float>(7, 8.0f);
  if (a.first != 7) return false;
  a = std::tuple<int, double>(9, 10.0);  // pair-like
  if (a.first != 9 || a.second != 10.0) return false;
  a = std::array<int, 2>{11, 12};
  if (a.first != 11 || a.second != 12.0) return false;

  // reference members: assignment writes through
  int x = 0, y = 0;
  std::pair<int&, int&> refs(x, y);
  refs = std::pair<int, int>(1, 2);
  if (x != 1 || y != 2) return false;
  const std::pair<int&, int&> crefs(x, y);
  crefs = std::pair<int, int>(3, 4);  // const operator=
  if (x != 3 || y != 4) return false;
  crefs = std::tuple<int, int>(5, 6);
  if (x != 5 || y != 6) return false;

  // swap
  std::pair<int, double> c(1, 1.5), d(2, 2.5);
  c.swap(d);
  if (c.first != 2 || d.second != 1.5) return false;
  swap(c, d);
  if (c.first != 1 || d.first != 2) return false;
  int p = 1, q = 2, u = 3, v = 4;
  const std::pair<int&, int&> e(p, q), f(u, v);
  e.swap(f);  // swaps the referents
  if (p != 3 || q != 4 || u != 1 || v != 2) return false;
  swap(e, f);
  if (p != 1 || u != 3) return false;
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  return 0;
}
