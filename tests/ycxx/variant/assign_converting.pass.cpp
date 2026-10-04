// [variant.assign]/11-16: converting assignment template<class T> operator=(T&&).
// Same FUN(Ti) selection as the converting constructor (narrowing excludes alternatives).
// Effects: (13.1) same alternative -> assign; (13.2) if is_nothrow_constructible_v<Tj,T> ||
// !is_nothrow_move_constructible_v<Tj> -> emplace<j>(forward<T>(t)); (13.3) otherwise
// emplace<j>(Tj(forward<T>(t))) -- i.e. construct a temporary then move.
// Remarks: noexcept = is_nothrow_assignable_v<Tj&,T> && is_nothrow_constructible_v<Tj,T>.
#include <variant>
#include <type_traits>
#include "check.hpp"

struct FromCStr { const char* p; constexpr FromCStr(const char* s) : p(s) {} };

struct ThrowingFromInt {
  static inline int moves = 0, conv = 0, assigns = 0;
  int v;
  ThrowingFromInt(int x) noexcept(false) : v(x) { ++conv; }
  ThrowingFromInt(ThrowingFromInt&& o) noexcept : v(o.v) { ++moves; }
  ThrowingFromInt& operator=(int x) { v = x; ++assigns; return *this; }
  ThrowingFromInt& operator=(ThrowingFromInt&&) = default;
};
struct NothrowFromInt {
  static inline int moves = 0, conv = 0;
  int v;
  NothrowFromInt(int x) noexcept : v(x) { ++conv; }
  NothrowFromInt(NothrowFromInt&& o) noexcept : v(o.v) { ++moves; }
  NothrowFromInt& operator=(int x) noexcept { v = x; return *this; }
};
struct NoAssign {
  NoAssign(int) {}
  NoAssign& operator=(int) = delete;
};

constexpr bool test() {
  std::variant<int, double> v;
  v = 2.5;
  if (v.index() != 1 || std::get<1>(v) != 2.5) return false;
  v = 3;
  if (v.index() != 0 || std::get<0>(v) != 3) return false;
  std::variant<float, long> w;
  int i = 9;
  w = i;  // int->float narrowing
  if (w.index() != 1 || std::get<1>(w) != 9) return false;
  std::variant<bool, FromCStr> b;
  b = "x";  // pointer->bool narrowing
  if (b.index() != 1) return false;
  b = false;
  if (b.index() != 0) return false;
  return true;
}
static_assert(test());

static_assert(!std::is_assignable_v<std::variant<int, int>&, int>);           // ambiguous
static_assert(!std::is_assignable_v<std::variant<long, long long>&, int>);    // ambiguous
static_assert(!std::is_assignable_v<std::variant<bool>&, int*>);              // narrowing
// Tj = NoAssign is excluded from the converting assignment (12.2), but `v = 1` is still valid:
// 1 converts implicitly to a variant and the move assignment is used instead.
static_assert(std::is_assignable_v<std::variant<NoAssign, double>&, int>);
static_assert(std::is_nothrow_assignable_v<std::variant<int, double>&, double>);
static_assert(!std::is_nothrow_assignable_v<std::variant<ThrowingFromInt, double>&, int>);
static_assert(std::is_nothrow_assignable_v<std::variant<NothrowFromInt, double>&, int>);

int main() {
  CHECK(test());
  // (13.3): Tj construction from int may throw, Tj move is nothrow -> temporary + move
  {
    std::variant<ThrowingFromInt, double> v(1.0);
    ThrowingFromInt::moves = ThrowingFromInt::conv = ThrowingFromInt::assigns = 0;
    v = 5;
    CHECK(v.index() == 0 && std::get<0>(v).v == 5);
    CHECK(ThrowingFromInt::conv == 1 && ThrowingFromInt::moves == 1);
    // (13.1): already holds Tj -> assign
    v = 6;
    CHECK(ThrowingFromInt::assigns == 1 && ThrowingFromInt::moves == 1 && std::get<0>(v).v == 6);
  }
  // (13.2): nothrow construction from int -> direct emplace, no move
  {
    std::variant<NothrowFromInt, double> v(1.0);
    NothrowFromInt::moves = NothrowFromInt::conv = 0;
    v = 5;
    CHECK(v.index() == 0 && std::get<0>(v).v == 5);
    CHECK(NothrowFromInt::conv == 1 && NothrowFromInt::moves == 0);
  }
  return 0;
}
