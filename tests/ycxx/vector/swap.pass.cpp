// [vector.capacity]/12-13: swap exchanges contents and capacity() in constant time.
// [container.reqmts]/65: swap does not move/copy/swap individual elements, and iterators
// keep referring to the same elements, now in the other container; /66.5-66.6: swap does
// not throw and does not invalidate references. Allocators are exchanged only if
// propagate_on_container_swap. [vector.syn]: non-member swap is
// noexcept(noexcept(x.swap(y))).
#include <vector>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

struct NoSwap {
  int v;
  NoSwap(int x) : v(x) {}
  NoSwap(const NoSwap&) = default;
  NoSwap& operator=(const NoSwap&) = default;
};
static int element_swaps = 0;
void swap(NoSwap&, NoSwap&) { ++element_swaps; }

static_assert(std::is_nothrow_swappable_v<std::vector<int>>);
using PV = std::vector<int, IdAlloc<int, false, false, true>>;
static_assert(noexcept(std::declval<PV&>().swap(std::declval<PV&>())));
static_assert(std::is_nothrow_swappable_v<PV>);

int main() {
  {
    std::vector<int> a{1, 2, 3};
    std::vector<int> b;
    b.reserve(100);
    auto acap = a.capacity(), bcap = b.capacity();
    int* p = &a[1];
    auto it = a.begin();
    a.swap(b);
    CHECK(a.empty() && b.size() == 3);
    CHECK(a.capacity() == bcap && b.capacity() == acap);
    CHECK(p == &b[1] && *p == 2 && it == b.begin());
    swap(a, b);
    CHECK(a.size() == 3 && p == &a[1]);
    std::swap(a, b);
    CHECK(b.size() == 3);
  }
  {
    std::vector<NoSwap> a{1, 2}, b{3};
    a.swap(b);
    using std::swap;
    swap(a, b);
    CHECK(element_swaps == 0);
    CHECK(a.size() == 2 && a[0].v == 1 && b[0].v == 3);
  }
  {
    PV a({1, 2}, IdAlloc<int, false, false, true>(1));
    PV b({3}, IdAlloc<int, false, false, true>(2));
    a.swap(b);
    CHECK(a.get_allocator().id == 2 && b.get_allocator().id == 1);
    CHECK(a.size() == 1 && b.size() == 2);
  }
  {
    std::vector<int, IdAlloc<int>> a({1, 2}, IdAlloc<int>(5));
    std::vector<int, IdAlloc<int>> b({3}, IdAlloc<int>(5));
    a.swap(b);
    CHECK(a.get_allocator().id == 5 && a.size() == 1 && b.size() == 2);
  }
  return 0;
}
