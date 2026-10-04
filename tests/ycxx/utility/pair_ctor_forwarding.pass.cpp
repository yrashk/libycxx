// [pairs.pair]/11-12: pair(U1&& x, U2&& y) "Initializes first with std::forward<U1>(x) and
// second with std::forward<U2>(y)." /14-16: for pair(pair<U1, U2>&), pair(const pair<U1,
// U2>&), pair(pair<U1, U2>&&), pair(const pair<U1, U2>&&) and pair(P&&) with pair-like P,
// "Let FWD(u) be static_cast<decltype(u)>(u)" and "Initializes first with get<0>(FWD(p)) and
// second with get<1>(FWD(p))." So each form passes the source's value category and constness
// through to the members' constructors (and get on a pair with a reference member yields that
// reference). All of these are constexpr.
#include <utility>
#include <array>
#include <tuple>
#include "check.hpp"

struct Src {
  int v = 0;
};
enum How { lv = 1, clv, rv, crv };
struct Rec {
  int how;
  int v;
  constexpr Rec(Src& s) : how(lv), v(s.v) {}
  constexpr Rec(const Src& s) : how(clv), v(s.v) {}
  constexpr Rec(Src&& s) : how(rv), v(s.v) {}
  constexpr Rec(const Src&& s) : how(crv), v(s.v) {}
};
using P = std::pair<Rec, Rec>;

constexpr bool both(const P& p, int a, int b) { return p.first.how == a && p.second.how == b; }

constexpr bool test() {
  Src s{1};
  const Src cs{2};
  // pair(U1&&, U2&&)
  if (!both(P(s, cs), lv, clv)) return false;
  if (!both(P(std::move(s), std::move(cs)), rv, crv)) return false;
  if (!both(P(cs, s), clv, lv)) return false;
  {
    P copy_list = {s, std::move(s)};  // implicit
    if (!both(copy_list, lv, rv)) return false;
  }
  if (P(s, cs).second.v != 2) return false;

  // pair(pair<U1, U2> in all four value categories)
  std::pair<Src, Src> ps{Src{3}, Src{4}};
  const std::pair<Src, Src>& cps = ps;
  if (!both(P(ps), lv, lv)) return false;
  if (!both(P(cps), clv, clv)) return false;
  if (!both(P(std::move(ps)), rv, rv)) return false;
  if (!both(P(std::move(cps)), crv, crv)) return false;
  if (P(ps).second.v != 4) return false;

  // a reference member: get<0> of an rvalue pair<Src&, Src> is Src&
  std::pair<Src&, Src> pr{s, Src{5}};
  if (!both(P(std::move(pr)), lv, rv)) return false;
  if (!both(P(pr), lv, lv)) return false;
  std::pair<const Src&, Src&&> pcr{cs, std::move(s)};
  if (!both(P(std::move(pcr)), clv, rv)) return false;
  if (!both(P(pcr), clv, lv)) return false;  // get<1>(lvalue) of an rvalue-reference member is Src&

  // pair-like: tuple and array
  std::tuple<Src, Src> t{Src{6}, Src{7}};
  const std::tuple<Src, Src>& ct = t;
  if (!both(P(t), lv, lv)) return false;
  if (!both(P(ct), clv, clv)) return false;
  if (!both(P(std::move(t)), rv, rv)) return false;
  if (!both(P(std::move(ct)), crv, crv)) return false;
  if (P(t).first.v != 6) return false;

  std::array<Src, 2> a{Src{8}, Src{9}};
  const std::array<Src, 2>& ca = a;
  if (!both(P(a), lv, lv)) return false;
  if (!both(P(ca), clv, clv)) return false;
  if (!both(P(std::move(a)), rv, rv)) return false;
  if (!both(P(std::move(ca)), crv, crv)) return false;
  if (P(a).second.v != 9) return false;

  std::tuple<Src&, const Src&> tr{s, cs};
  if (!both(P(std::move(tr)), lv, clv)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
