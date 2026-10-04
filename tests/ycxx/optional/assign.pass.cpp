// [optional.assign]: operator=(nullopt_t) noexcept; copy/move assignment (Tables 67/68:
// assign when both engaged, construct when only rhs engaged, destroy when only *this);
// move noexcept = nothrow move-assignable && nothrow move-constructible; operator=(U&&)
// constraints incl. /14.2 (scalar T and same decayed U excluded, so `o = {}` disengages);
// converting assignment from optional<U> with its constraints /19, /24.
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counts {
  static inline int ctor = 0, assign = 0, dtor = 0;
  int v;
  Counts(int x) : v(x) { ++ctor; }
  Counts(const Counts& o) : v(o.v) { ++ctor; }
  Counts& operator=(const Counts& o) { v = o.v; ++assign; return *this; }
  Counts& operator=(int x) { v = x; ++assign; return *this; }
  ~Counts() { ++dtor; }
  static void reset() { ctor = assign = dtor = 0; }
};
struct ThrowingMoveAssign {
  ThrowingMoveAssign() = default;
  ThrowingMoveAssign(ThrowingMoveAssign&&) noexcept = default;
  ThrowingMoveAssign& operator=(ThrowingMoveAssign&&) noexcept(false) { return *this; }
};
struct FromOpt {
  FromOpt(int) {}
  FromOpt(const std::optional<int>&) {}
  FromOpt& operator=(int) { return *this; }
};

static_assert(noexcept(std::declval<std::optional<Counts>&>() = std::nullopt));
static_assert(std::is_nothrow_move_assignable_v<std::optional<int>>);
static_assert(!std::is_nothrow_move_assignable_v<std::optional<ThrowingMoveAssign>>);
static_assert(std::is_assignable_v<std::optional<long>&, const std::optional<int>&>);
static_assert(!std::is_assignable_v<std::optional<int*>&, std::optional<long>>);

constexpr bool test() {
  std::optional<int> o(3);
  o = {};  // /14.2: selects operator=(optional&&) with an empty optional, not o = int{}
  if (o.has_value()) return false;
  o = 4;
  if (!o || *o != 4) return false;
  o = std::nullopt;
  if (o) return false;
  std::optional<int> a(1), b;
  a = b;
  if (a) return false;
  b = 5;
  a = b;
  if (*a != 5) return false;
  a = std::optional<int>(6);
  if (*a != 6) return false;
  std::optional<long> l;
  l = a;  // optional<U> const&
  if (*l != 6) return false;
  l = std::optional<int>();  // optional<U>&&
  if (l) return false;
  l = std::optional<short>(2);
  if (*l != 2) return false;
  // braced initializer for a non-scalar T assigns T{...}
  struct P { int x, y; };
  std::optional<P> p;
  p = P{1, 2};
  if (!p || p->y != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  {
    std::optional<Counts> a(1), b(2);
    Counts::reset();
    a = b;  // both engaged: assignment
    CHECK(Counts::assign == 1 && Counts::ctor == 0 && Counts::dtor == 0);
    std::optional<Counts> c;
    Counts::reset();
    c = b;  // only rhs engaged: construction
    CHECK(Counts::ctor == 1 && Counts::assign == 0 && c->v == 2);
    Counts::reset();
    c = std::optional<Counts>();  // only *this engaged: destroy
    CHECK(!c && Counts::dtor == 1);
    Counts::reset();
    a = 7;  // operator=(U&&) on engaged: assigns
    CHECK(Counts::assign == 1 && Counts::ctor == 0 && a->v == 7);
    Counts::reset();
    c = 8;  // operator=(U&&) on empty: constructs
    CHECK(Counts::ctor == 1 && Counts::assign == 0 && c->v == 8);
    Counts::reset();
    a = std::nullopt;
    CHECK(!a && Counts::dtor == 1);
  }
  {
    // /19.3: T constructible from optional<U> -> converting optional<U> assignment excluded,
    // so opt = optional<int>{} goes through operator=(U&&) and stays engaged.
    std::optional<FromOpt> f;
    f = std::optional<int>();
    CHECK(f.has_value());
  }
  return 0;
}
