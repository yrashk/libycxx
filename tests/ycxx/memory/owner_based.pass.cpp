// [util.smartptr.shared.obs]/19-22, [util.smartptr.weak.obs]: owner_before is a strict weak
// order under which two pointers are equivalent iff owner_equal; owner_equal(b) is true iff
// *this and b share ownership or are both empty; owner_hash() agrees with owner_equal.
// [util.smartptr.ownerless]: owner_less<void> compares shared and weak pointers by
// owner_before. [util.smartptr.owner.hash], [util.smartptr.owner.equal]: owner_hash()(x)
// returns x.owner_hash(), owner_equal()(x, y) returns x.owner_equal(y); both are
// transparent.
#include <memory>
#include <functional>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
};

template <class F>
concept transparent = requires { typename F::is_transparent; };
static_assert(transparent<std::owner_hash>);
static_assert(transparent<std::owner_equal>);
static_assert(transparent<std::owner_less<void>>);

int main() {
  auto owner = std::make_shared<P>();
  std::shared_ptr<int> pa(owner, &owner->a), pb(owner, &owner->b);
  std::weak_ptr<int> wa = pa;
  auto other = std::make_shared<int>(1);
  std::shared_ptr<int> e1, e2;
  std::weak_ptr<int> we;

  CHECK(pa.owner_equal(pb) && pb.owner_equal(owner) && pa.owner_equal(wa) && wa.owner_equal(pb));
  CHECK(!pa.owner_equal(other) && !wa.owner_equal(other));
  CHECK(e1.owner_equal(e2) && e1.owner_equal(we) && we.owner_equal(e1));
  CHECK(!e1.owner_equal(pa));
  CHECK(!pa.owner_before(pb) && !pb.owner_before(pa));
  CHECK(pa.owner_before(other) != other.owner_before(pa));
  CHECK(!e1.owner_before(e2) && !we.owner_before(e1));

  CHECK(pa.owner_hash() == pb.owner_hash() && pa.owner_hash() == wa.owner_hash());
  CHECK(e1.owner_hash() == we.owner_hash());
  std::owner_hash oh;
  CHECK(oh(pa) == pa.owner_hash() && oh(wa) == wa.owner_hash());
  static_assert(noexcept(oh(pa)));
  std::owner_equal oe;
  CHECK(oe(pa, pb) && oe(pa, wa) && oe(wa, pa) && oe(wa, wa) && !oe(pa, other));
  static_assert(noexcept(oe(pa, pb)));
  std::owner_less<> ol;
  CHECK(!ol(pa, pb) && !ol(wa, pb) && ol(pa, other) != ol(other, pa));
  std::owner_less<std::shared_ptr<int>> ols;
  CHECK(!ols(pa, pb) && !ols(pa, wa));

  // expired weak_ptr still owner-equal to... nothing alive, but to itself
  std::weak_ptr<int> wo = other;
  other.reset();
  CHECK(wo.owner_equal(wo) && !wo.owner_equal(e1));  // expired is not empty
  return 0;
}
