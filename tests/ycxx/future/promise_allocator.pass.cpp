// [futures.promise]/4: promise(allocator_arg_t, const Allocator& a) "uses the
// allocator a to allocate memory for the shared state".
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
  allocations = deallocations = 0;
  {
    std::promise<int> p(std::allocator_arg, CountingAlloc<int>());
    CHECK(allocations >= 1);
    auto f = p.get_future();
    p.set_value(3);
    CHECK(f.get() == 3);
  }
  CHECK(deallocations == allocations && allocations >= 1);
  return 0;
}
