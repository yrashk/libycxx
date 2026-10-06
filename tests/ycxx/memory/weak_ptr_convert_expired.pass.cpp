// [util.smartptr.weak.const]/3-/5: weak_ptr(const weak_ptr<Y>& r) and weak_ptr(const
// shared_ptr<Y>&) (Constraints: Y* is compatible with T*): "If r is empty, constructs an empty
// weak_ptr object; otherwise, constructs a weak_ptr object that shares ownership with r and
// stores a copy of the pointer stored in r. Postconditions: use_count() == r.use_count()."
// /6-/8: the move constructor and the converting move constructor leave r empty.
// [util.smartptr.weak.assign]/1-/6: copy and converting assignment are weak_ptr(r).swap(*this),
// the move assignments weak_ptr(std::move(r)).swap(*this).
// [util.smartptr.weak.mod]/1-/2: swap exchanges, reset empties; [util.smartptr.weak.spec]/1:
// non-member swap.
// The conversions are from weak_ptrs whose object has been destroyed and deallocated, to a
// virtual base class: they must still share ownership (owner-equivalent, use_count 0) without
// reading the destroyed object (adjusting a pointer to a virtual base would read it; run under
// ASan this is a heap-use-after-free).
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct VB {
  int v = 1;
  virtual ~VB() = default;
};
struct Mid : virtual VB {
  int m = 2;
};
struct D : Mid, virtual VB {
  int d = 3;
};

static_assert(std::is_convertible_v<std::weak_ptr<D>, std::weak_ptr<VB>>);
static_assert(std::is_convertible_v<std::weak_ptr<D>&&, std::weak_ptr<const VB>>);
static_assert(!std::is_convertible_v<std::weak_ptr<VB>, std::weak_ptr<D>>);
static_assert(!std::is_constructible_v<std::weak_ptr<int>, std::weak_ptr<long>>);
static_assert(std::is_convertible_v<std::weak_ptr<int[3]>, std::weak_ptr<int[]>>);
static_assert(std::is_convertible_v<std::weak_ptr<int[3]>, std::weak_ptr<const int[3]>>);
static_assert(!std::is_convertible_v<std::weak_ptr<int[]>, std::weak_ptr<int[3]>>);

bool owner_equivalent(const auto& a, const auto& b) { return !a.owner_before(b) && !b.owner_before(a); }

int main() {
  {
    std::shared_ptr<D> sp(new D);  // the object's storage is freed with the last shared_ptr
    std::weak_ptr<D> wd = sp;
    std::weak_ptr<VB> live = wd;  // conversion while alive: the base subobject
    CHECK(live.use_count() == 1);
    CHECK(live.lock().get() == static_cast<VB*>(sp.get()));
    sp.reset();
    CHECK(wd.expired());

    std::weak_ptr<VB> wb = wd;  // copy conversion of an expired weak_ptr
    CHECK(wb.expired() && wb.use_count() == 0 && !wb.lock());
    CHECK(owner_equivalent(wb, wd) && owner_equivalent(wb, live));
    CHECK(wb.owner_equal(wd) && wb.owner_hash() == wd.owner_hash());

    std::weak_ptr<const VB> wc;
    wc = wd;  // converting copy assignment
    CHECK(wc.expired() && owner_equivalent(wc, wd));

    std::weak_ptr<D> wd2 = wd;
    std::weak_ptr<VB> wm = std::move(wd2);  // converting move: wd2 becomes empty
    CHECK(wm.expired() && owner_equivalent(wm, wd));
    CHECK(wd2.use_count() == 0 && owner_equivalent(wd2, std::weak_ptr<D>()) && !owner_equivalent(wd2, wd));

    std::weak_ptr<D> wd3 = wd;
    std::weak_ptr<Mid> wmid;
    wmid = std::move(wd3);  // converting move assignment
    CHECK(owner_equivalent(wmid, wd) && owner_equivalent(wd3, std::weak_ptr<Mid>()));
  }
  {
    // the same through make_shared (storage kept until the last weak_ptr is gone)
    auto sp = std::make_shared<D>();
    std::weak_ptr<D> wd = sp;
    sp.reset();
    std::weak_ptr<VB> wb(wd);
    CHECK(wb.expired() && owner_equivalent(wb, wd));
  }
  {
    // empty sources give empty results
    std::weak_ptr<D> e;
    std::weak_ptr<VB> we = e;
    CHECK(we.use_count() == 0 && owner_equivalent(we, std::weak_ptr<VB>()));
    std::shared_ptr<D> null_owner(static_cast<D*>(nullptr));  // owns a null pointer: not empty
    std::weak_ptr<VB> wn = null_owner;
    CHECK(wn.use_count() == 1 && !owner_equivalent(wn, std::weak_ptr<VB>()));
  }
  {
    // swap, reset, non-member swap
    auto a = std::make_shared<int>(1), b = std::make_shared<int>(2);
    std::weak_ptr<int> wa = a, wb = b;
    wa.swap(wb);
    CHECK(*wa.lock() == 2 && *wb.lock() == 1);
    swap(wa, wb);
    CHECK(*wa.lock() == 1 && *wb.lock() == 2);
    static_assert(noexcept(wa.swap(wb)) && noexcept(wa.reset()) && noexcept(std::swap(wa, wb)));
    wa.reset();
    CHECK(wa.use_count() == 0 && wa.expired() && a.use_count() == 1);
    std::weak_ptr<int> self = b;
    std::weak_ptr<int>& alias = self;
    self = alias;
    CHECK(self.lock() == b);
    self = std::move(alias);  // weak_ptr(std::move(r)).swap(*this): the value is kept
    CHECK(self.lock() == b);
  }
  return 0;
}
