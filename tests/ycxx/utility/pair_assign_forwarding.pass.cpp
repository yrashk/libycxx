// [pairs.pair]/26-27: operator=(const pair<U1, U2>& p) "Assigns p.first to first and
// p.second to second." /39-40: operator=(pair<U1, U2>&& p) "Assigns std::forward<U1>(p.first)
// first and std::forward<U2>(p.second) to second." /42-43: operator=(P&& p) for pair-like P
// (different-from<P, pair>) "Assigns get<0>(std::forward<P>(p)) to first and
// get<1>(std::forward<P>(p)) to second." /29-30, /45-46, /48-49: the const-qualified forms
// (through reference members) do the same. So the source's value category reaches the
// members' assignment operators. A non-const lvalue pair<U1, U2> is itself pair-like, and
// binding it to P&& (P = pair<U1, U2>&) is a better reference binding than to const pair<U1,
// U2>& ([over.ics.rank]/3.2.6), so its members are assigned as lvalues.
#include <utility>
#include <array>
#include <tuple>
#include "check.hpp"

struct Src {};
enum How { none, lv, clv, rv, crv };
struct Sink {
  int how = none;
  constexpr Sink& operator=(Src&) { how = lv; return *this; }
  constexpr Sink& operator=(const Src&) { how = clv; return *this; }
  constexpr Sink& operator=(Src&&) { how = rv; return *this; }
  constexpr Sink& operator=(const Src&&) { how = crv; return *this; }
};
using P = std::pair<Sink, Sink>;

constexpr bool both(const P& p, int a, int b) { return p.first.how == a && p.second.how == b; }
constexpr bool both(const std::pair<Sink&, Sink&>& p, int a, int b) { return p.first.how == a && p.second.how == b; }

constexpr bool test() {
  P d;
  std::pair<Src, Src> s;
  const std::pair<Src, Src>& cs = s;
  d = cs;
  if (!both(d, clv, clv)) return false;
  d = std::move(s);
  if (!both(d, rv, rv)) return false;
  d = s;  // pair-like P&& with P = pair<Src, Src>&
  if (!both(d, lv, lv)) return false;

  Src x;
  std::pair<Src&, Src> sr(x, Src{});
  d = std::move(sr);  // std::forward<Src&>(p.first) is an lvalue
  if (!both(d, lv, rv)) return false;
  std::pair<const Src&, Src&&> scr(x, std::move(x));
  d = std::move(scr);
  if (!both(d, clv, rv)) return false;

  std::tuple<Src, Src> t;
  const std::tuple<Src, Src>& ct = t;
  d = t;
  if (!both(d, lv, lv)) return false;
  d = ct;
  if (!both(d, clv, clv)) return false;
  d = std::move(t);
  if (!both(d, rv, rv)) return false;
  d = std::move(ct);
  if (!both(d, crv, crv)) return false;

  std::array<Src, 2> a;
  d = a;
  if (!both(d, lv, lv)) return false;
  d = std::move(a);
  if (!both(d, rv, rv)) return false;
  d = std::move(std::as_const(a));
  if (!both(d, crv, crv)) return false;

  // const-qualified assignment through reference members
  Sink m1, m2;
  const std::pair<Sink&, Sink&> refs(m1, m2);
  refs = cs;
  if (!both(refs, clv, clv)) return false;
  refs = std::move(s);
  if (!both(refs, rv, rv)) return false;
  refs = std::move(t);
  if (!both(refs, rv, rv)) return false;
  refs = ct;
  if (!both(refs, clv, clv)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
