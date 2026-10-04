// [pairs.pair]/11-17: the converting constructors initialize each member from the forwarded
// source member (or get<i>(FWD(p)) for pair-like sources). So conversions between element
// types happen member-wise, move-only members are moved from rvalue sources (and the source
// is left moved-from), and reference members bind to the source objects themselves.
#include <utility>
#include <array>
#include <tuple>
#include "check.hpp"

struct MoveOnly {
  int v;
  constexpr explicit MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};
struct Holder {
  int v;
  constexpr Holder(MoveOnly&& m) : v(m.v) { m.v = -2; }
};

constexpr bool test() {
  // arithmetic conversions
  std::pair<int, float> pi{3, 1.5f};
  std::pair<long, double> pl(pi);
  if (pl.first != 3 || pl.second != 1.5) return false;
  std::pair<long, double> pl2 = std::pair<short, float>(4, 2.5f);
  if (pl2.first != 4 || pl2.second != 2.5) return false;
  std::pair<long, double> pt = std::tuple<int, float>(5, 3.5f);
  if (pt.first != 5 || pt.second != 3.5) return false;
  std::pair<long, long> pa = std::array<int, 2>{6, 7};
  if (pa.first != 6 || pa.second != 7) return false;

  // move-only members
  std::pair<MoveOnly, int> src(MoveOnly(8), 9);
  std::pair<Holder, long> dst(std::move(src));
  if (dst.first.v != 8 || dst.second != 9 || src.first.v != -2) return false;
  std::tuple<MoveOnly, int> tsrc(MoveOnly(10), 11);
  std::pair<MoveOnly, int> fromt(std::move(tsrc));
  if (fromt.first.v != 10 || std::get<0>(tsrc).v != -1) return false;
  std::pair<MoveOnly, MoveOnly> uu(MoveOnly(12), MoveOnly(13));
  if (uu.first.v != 12 || uu.second.v != 13) return false;

  // reference members bind to the source objects
  int x = 1;
  long y = 2;
  std::pair<int&, long&> refs(x, y);
  refs.first = 20;
  refs.second = 30;
  if (x != 20 || y != 30) return false;
  std::pair<int, long> vals{40, 50};
  std::pair<int&, long&> refs2(vals);
  if (&refs2.first != &vals.first || &refs2.second != &vals.second) return false;
  std::pair<const int&, const long&> crefs(std::as_const(vals));
  if (&crefs.first != &vals.first) return false;
  std::pair<int&&, long> rr(std::move(vals));
  if (&rr.first != &vals.first) return false;
  std::tuple<int, int> tv{1, 2};
  std::pair<int&, int&> fromtup(tv);
  if (&fromtup.first != &std::get<0>(tv) || &fromtup.second != &std::get<1>(tv)) return false;
  std::array<int, 2> arr{3, 4};
  std::pair<const int&, int&> fromarr(arr);
  if (&fromarr.first != &arr[0] || &fromarr.second != &arr[1]) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
