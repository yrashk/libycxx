// [futures.task.members]/3-7: template<class F, class Allocator> explicit packaged_task(
// allocator_arg_t, const Allocator& a, F&& f): "Uses a2 to allocate storage for the shared
// state and stores a copy of a2 in the shared state." /28: reset() reconstructs with
// packaged_task(allocator_arg, a, std::move(f)) "where ... a is the allocator stored in the
// shared state".
#include <future>
#include <memory>
#include <cstddef>
#include "check.hpp"

static int allocations = 0, deallocations = 0;

template<class T>
struct CountingAlloc {
  using value_type = T;
  CountingAlloc() = default;
  template<class U> CountingAlloc(const CountingAlloc<U>&) {}
  T* allocate(std::size_t n) { ++allocations; return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) { ++deallocations; std::allocator<T>().deallocate(p, n); }
  template<class U> bool operator==(const CountingAlloc<U>&) const { return true; }
};

int main() {
  {
    std::packaged_task<int()> t(std::allocator_arg, CountingAlloc<int>(), [] { return 4; });
    CHECK(allocations >= 1);
    auto f = t.get_future();
    t();
    CHECK(f.get() == 4);
    int before = allocations;
    t.reset();
    CHECK(allocations > before);  // the new shared state uses the stored allocator
    auto f2 = t.get_future();
    t();
    CHECK(f2.get() == 4);
  }
  CHECK(deallocations == allocations);

  return 0;
}
