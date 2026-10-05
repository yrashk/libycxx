// [util.smartptr.shared.create]: make_shared<T>(args) constructs T(std::forward<Args>(args)
// ...) (parenthesized, aggregates included) and returns a shared_ptr owning it
// (use_count() == 1); allocate_shared uses a rebound copy of the allocator for the storage
// and destroys/deallocates it at the end. Array forms: make_shared<U[]>(N) and
// make_shared<U[N]>() value-initialize the elements; (N, u) / (u) initialize every element
// from u; elements are destroyed in the reverse order of their construction.
// make_shared_for_overwrite default-initializes.
// REQUIRES: exceptions
#include <memory>
#include <string>
#include <type_traits>
#include "test_allocators.hpp"
#include "check.hpp"

struct Agg {
  int a;
  std::string b;
};
static int order[8];
static int order_n = 0;
struct Tracked {
  static inline int next = 0;
  int id;
  Tracked() : id(next++) {}
  ~Tracked() { order[order_n++] = id; }
};

int main() {
  {
    auto p = std::make_shared<int>();
    CHECK(*p == 0 && p.use_count() == 1);
    auto q = std::make_shared<Agg>(1, "x");
    CHECK(q->a == 1 && q->b == "x");
    auto s = std::make_shared<std::string>(3, 'z');
    CHECK(*s == "zzz");
    static_assert(std::is_same_v<decltype(std::make_shared<const int>(1)), std::shared_ptr<const int>>);
  }
  {
    auto a = std::make_shared<int[]>(4);
    CHECK(a[0] == 0 && a[3] == 0);
    auto b = std::make_shared<int[3]>();
    static_assert(std::is_same_v<decltype(b), std::shared_ptr<int[3]>>);
    CHECK(b[2] == 0);
    auto c = std::make_shared<int[]>(3, 7);
    CHECK(c[0] == 7 && c[2] == 7);
    auto d = std::make_shared<int[2]>(5);
    CHECK(d[0] == 5 && d[1] == 5);
    auto e = std::make_shared<int[][2]>(2, {1, 2});  // u is an array: each element copies it
    CHECK(e[1][0] == 1 && e[1][1] == 2);
    auto f = std::make_shared_for_overwrite<int[]>(2);
    f[0] = 1;
    CHECK(f[0] == 1);
    auto g = std::make_shared_for_overwrite<int>();
    *g = 3;
    CHECK(*g == 3);
  }
  {
    Tracked::next = 0;
    order_n = 0;
    { auto t = std::make_shared<Tracked[]>(3); }
    CHECK(order_n == 3 && order[0] == 2 && order[1] == 1 && order[2] == 0);
  }
  {
    alloc_counters = {};
    {
      auto p = std::allocate_shared<int>(CountingAlloc<int>(), 5);
      CHECK(*p == 5 && alloc_counters.allocations >= 1);
      int n = alloc_counters.allocations;
      auto q = std::allocate_shared<int[]>(CountingAlloc<int>(), 3);
      CHECK(q[2] == 0 && alloc_counters.allocations > n);
      n = alloc_counters.allocations;
      auto r = std::allocate_shared_for_overwrite<int>(CountingAlloc<int>());
      CHECK(alloc_counters.allocations > n);
      CHECK(alloc_counters.deallocations == 0);
    }
    CHECK(alloc_counters.deallocations == alloc_counters.allocations && alloc_counters.outstanding == 0);
  }
  return 0;
}
