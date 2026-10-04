// [mem.res.class], [mem.res.public], [mem.res.eq]: allocate(bytes, alignment = max_align) calls
// do_allocate, deallocate calls do_deallocate with the same arguments, is_equal calls
// do_is_equal; operator==(a, b) is &a == &b || a.is_equal(b) (and != is its negation).
// memory_resource is an abstract class with a virtual destructor, copyable.
#include <memory_resource>
#include <cstddef>
#include <type_traits>
#include "check.hpp"
#include "recording_resource.hpp"

static_assert(std::is_abstract_v<std::pmr::memory_resource>);
static_assert(std::has_virtual_destructor_v<std::pmr::memory_resource>);
static_assert(std::is_copy_assignable_v<RecordingResource>);
static_assert(noexcept(std::declval<const std::pmr::memory_resource&>().is_equal(std::declval<const std::pmr::memory_resource&>())));
static_assert(noexcept(std::declval<const std::pmr::memory_resource&>() == std::declval<const std::pmr::memory_resource&>()));

struct AlwaysEqual : RecordingResource {
 private:
  bool do_is_equal(const std::pmr::memory_resource&) const noexcept override { return true; }
};

int main() {
  RecordingResource r;
  void* p = r.allocate(40);
  CHECK(r.allocs == 1 && r.last_bytes == 40 && r.last_align == alignof(std::max_align_t));
  r.deallocate(p, 40);
  CHECK(r.deallocs == 1 && r.last_bytes == 40 && r.last_align == alignof(std::max_align_t));
  void* q = r.allocate(64, 32);
  CHECK(r.last_align == 32 && reinterpret_cast<std::size_t>(q) % 32 == 0);
  r.deallocate(q, 64, 32);
  CHECK(r.last_align == 32 && r.outstanding == 0);

  RecordingResource other;
  CHECK(r == r && !(r == other) && r != other);
  CHECK(r.is_equal(r) && !r.is_equal(other));
  AlwaysEqual ae;
  CHECK(ae == other);    // ae.is_equal(other) is true
  CHECK(!(other == ae)); // other.is_equal(ae) is false: no symmetry is imposed
  return 0;
}
