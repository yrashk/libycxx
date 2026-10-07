// [memory.syn]: namespace pmr { template<class T> using indirect = indirect<T,
// polymorphic_allocator<T>>; template<class T> using polymorphic = polymorphic<T,
// polymorphic_allocator<T>>; }, declared by <memory>. The owned object is allocated from the
// memory resource of the polymorphic_allocator ([indirect.general]/3, [polymorphic.general]/3).
#include <memory>
#include <memory_resource>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<std::pmr::indirect<int>, std::indirect<int, std::pmr::polymorphic_allocator<int>>>);
static_assert(std::is_same_v<std::pmr::polymorphic<int>,
                             std::polymorphic<int, std::pmr::polymorphic_allocator<int>>>);

struct base {
  int v;
};

struct counting_resource : std::pmr::memory_resource {
  int allocations = 0;
  void* do_allocate(std::size_t n, std::size_t a) override {
    ++allocations;
    return std::pmr::new_delete_resource()->allocate(n, a);
  }
  void do_deallocate(void* p, std::size_t n, std::size_t a) override {
    std::pmr::new_delete_resource()->deallocate(p, n, a);
  }
  bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
};

int main() {
  counting_resource r;
  {
    std::pmr::indirect<int> i(std::allocator_arg, &r, 42);
    CHECK(*i == 42 && r.allocations == 1);
    CHECK(i.get_allocator().resource() == &r);
    std::pmr::polymorphic<base> p(std::allocator_arg, &r, base{7});
    CHECK(p->v == 7 && r.allocations == 2);
    CHECK(p.get_allocator().resource() == &r);
  }
  return 0;
}
