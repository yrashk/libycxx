// [unique.ptr.special]: x == y is x.get() == y.get(); x < y is less<CT>()(x.get(), y.get())
// with CT the common pointer type; >, <=, >= are derived from <; x <=> y is
// compare_three_way()(x.get(), y.get()) (constrained on three_way_comparable_with). With
// nullptr: x == nullptr is !x (noexcept); the ordering operators use less<pointer> against
// nullptr; x <=> nullptr compares with static_cast<pointer>(nullptr).
#include <memory>
#include <compare>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  int* arr = new int[2]{0, 0};
  // Non-owning deleters so that two unique_ptrs can point into one array.
  struct NoDel {
    void operator()(int*) const {}
  };
  std::unique_ptr<int, NoDel> lo(arr), hi(arr + 1), lo2(arr), null;
  CHECK(lo == lo2 && !(lo != lo2) && lo != hi);
  CHECK(lo < hi && hi > lo && lo <= lo2 && lo >= lo2 && !(hi < lo) && hi >= lo);
  CHECK((lo <=> hi) == std::strong_ordering::less);
  CHECK((hi <=> lo) == std::strong_ordering::greater);
  CHECK((lo <=> lo2) == std::strong_ordering::equal);
  static_assert(std::is_same_v<decltype(lo <=> hi), std::strong_ordering>);

  CHECK(null == nullptr && nullptr == null && !(lo == nullptr) && lo != nullptr && nullptr != lo);
  static_assert(noexcept(null == nullptr));
  static_assert(noexcept(nullptr == null));
  CHECK(!(null < nullptr) && !(nullptr < null) && null <= nullptr && null >= nullptr);
  CHECK(nullptr < lo && lo > nullptr && !(lo < nullptr) && nullptr <= lo && lo >= nullptr);
  CHECK((null <=> nullptr) == std::strong_ordering::equal);
  CHECK((lo <=> nullptr) == std::strong_ordering::greater);
  CHECK((nullptr <=> lo) == std::strong_ordering::less);
  delete[] arr;

  // Heterogeneous comparison through the common pointer type.
  Derived* d = new Derived;
  std::unique_ptr<Derived> ud(d);
  std::unique_ptr<Base> ub;
  CHECK(ud != ub && !(ud == ub));
  CHECK(ub < ud || ud < ub);
  CHECK((ub <=> ud) != 0);
  std::unique_ptr<Base> ub2(ud.release());
  std::unique_ptr<const Base> cb;
  CHECK(cb < ub2 && (cb <=> ub2) < 0);
  return 0;
}
