// [mem.res.global]: new_delete_resource() and null_memory_resource() return pointers to
// static-duration resources, the same each call, equal only to themselves; the null resource's
// allocate always throws bad_alloc and deallocate has no effect. The default resource is
// initially new_delete_resource(); set_default_resource(r) sets it (nullptr restores
// new_delete_resource()) and returns the previous value; get_default_resource() returns it.
// All four are noexcept.
#include <memory_resource>
#include <new>
#include "check.hpp"
#include "recording_resource.hpp"

static_assert(noexcept(std::pmr::new_delete_resource()) && noexcept(std::pmr::null_memory_resource()));
static_assert(noexcept(std::pmr::get_default_resource()) && noexcept(std::pmr::set_default_resource(nullptr)));

int main() {
  std::pmr::memory_resource* nd = std::pmr::new_delete_resource();
  CHECK(nd != nullptr && nd == std::pmr::new_delete_resource());
  std::pmr::memory_resource* nul = std::pmr::null_memory_resource();
  CHECK(nul != nullptr && nul == std::pmr::null_memory_resource() && nul != nd);
  CHECK(*nd == *nd && !(*nd == *nul) && !nd->is_equal(*nul));
  RecordingResource r;
  CHECK(!nd->is_equal(r) && !nul->is_equal(r));

  void* p = nd->allocate(100, 16);
  CHECK(p != nullptr);
  nd->deallocate(p, 100, 16);

  bool threw = false;
  try {
    (void)nul->allocate(1);
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  CHECK(threw);
  nul->deallocate(nullptr, 0);  // no effect

  CHECK(std::pmr::get_default_resource() == nd);
  CHECK(std::pmr::set_default_resource(&r) == nd);
  CHECK(std::pmr::get_default_resource() == &r);
  std::pmr::polymorphic_allocator<int> a;  // picks up the default resource
  CHECK(a.resource() == &r);
  CHECK(std::pmr::set_default_resource(nullptr) == &r);
  CHECK(std::pmr::get_default_resource() == nd);
  return 0;
}
