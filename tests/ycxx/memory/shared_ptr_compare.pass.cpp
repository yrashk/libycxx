// [util.smartptr.shared.cmp]: a == b is a.get() == b.get(); a <=> b is
// compare_three_way()(a.get(), b.get()); a == nullptr is !a; a <=> nullptr is
// compare_three_way()(a.get(), static_cast<element_type*>(nullptr)). Comparison is by the
// stored pointer, not by ownership: aliases compare by what they point to.
#include <memory>
#include <compare>
#include <type_traits>
#include "check.hpp"

struct P {
  int a = 0;
  int b = 0;
};

int main() {
  auto owner = std::make_shared<P>();
  std::shared_ptr<int> pa(owner, &owner->a), pb(owner, &owner->b), pa2(owner, &owner->a), empty;
  CHECK(pa == pa2 && pa != pb);
  CHECK((pa <=> pb) == std::strong_ordering::less && pa < pb && pb > pa && pa <= pa2 && pb >= pa);
  static_assert(std::is_same_v<decltype(pa <=> pb), std::strong_ordering>);
  CHECK(empty == nullptr && nullptr == empty && pa != nullptr);
  CHECK((empty <=> nullptr) == 0 && (pa <=> nullptr) > 0 && (nullptr <=> pa) < 0);
  CHECK(pa > nullptr && nullptr < pa && !(empty < nullptr));
  static_assert(noexcept(pa == pb));
  static_assert(noexcept(pa == nullptr));
  std::shared_ptr<const int> cpa = pa;
  CHECK(cpa == pa && (cpa <=> pb) < 0);
  // owner-based comparisons see one owner
  CHECK(!pa.owner_before(pb) && !pb.owner_before(pa));
  CHECK(pa.owner_before(empty) || empty.owner_before(pa));
  return 0;
}
