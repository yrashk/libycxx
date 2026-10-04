// [mem.res.monotonic.buffer]: monotonic_buffer_resource serves allocations from its current
// buffer (an initial buffer if one is given), going upstream only when it does not fit;
// do_deallocate has no effect; release() returns everything to upstream (even blocks not
// deallocated) and resets to the initial buffer; the destructor calls release();
// upstream_resource() returns the upstream; do_is_equal is identity. Not copyable.
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "recording_resource.hpp"

using M = std::pmr::monotonic_buffer_resource;
static_assert(!std::is_copy_constructible_v<M> && !std::is_copy_assignable_v<M>);
static_assert(std::is_base_of_v<std::pmr::memory_resource, M>);
static_assert(!std::is_convertible_v<std::pmr::memory_resource*, M>);  // explicit
static_assert(!std::is_convertible_v<std::size_t, M>);

bool inside(void* p, void* buf, std::size_t n) {
  auto a = reinterpret_cast<std::uintptr_t>(p), b = reinterpret_cast<std::uintptr_t>(buf);
  return a >= b && a < b + n;
}

int main() {
  RecordingResource up;
  {
    alignas(16) static unsigned char buf[256];
    M m(buf, sizeof buf, &up);
    CHECK(m.upstream_resource() == &up);
    void* a = m.allocate(32, 8);
    void* b = m.allocate(32, 8);
    CHECK(inside(a, buf, sizeof buf) && inside(b, buf, sizeof buf) && a != b);
    CHECK(up.allocs == 0);  // served from the initial buffer
    void* c = m.allocate(16, 16);
    CHECK(reinterpret_cast<std::uintptr_t>(c) % 16 == 0);
    m.deallocate(a, 32, 8);  // no effect
    CHECK(up.deallocs == 0);
    void* big = m.allocate(1000, 8);  // does not fit: upstream
    CHECK(!inside(big, buf, sizeof buf) && up.allocs == 1 && up.last_bytes >= 1000 && up.last_align >= 8);
    void* big2 = m.allocate(2000, 64);
    CHECK(reinterpret_cast<std::uintptr_t>(big2) % 64 == 0);
    m.release();
    CHECK(up.deallocs == up.allocs && up.outstanding == 0);
    void* again = m.allocate(32, 8);  // the initial buffer is used again
    CHECK(inside(again, buf, sizeof buf));
    CHECK(m == m && !(m == up) && !m.is_equal(up));
  }
  CHECK(up.outstanding == 0);

  {
    M m(&up);
    int before = up.allocs;
    for (int i = 0; i < 100; ++i) (void)m.allocate(24, 8);
    CHECK(up.allocs > before);
    CHECK(up.allocs - before < 100);  // memory comes in chunks, not one call per allocation
  }
  CHECK(up.outstanding == 0);  // the destructor released everything

  {
    std::pmr::set_default_resource(&up);
    M m;  // default upstream
    CHECK(m.upstream_resource() == &up);
    M sized(128);
    CHECK(sized.upstream_resource() == &up);
    (void)sized.allocate(100);
    std::pmr::set_default_resource(nullptr);
  }
  CHECK(up.outstanding == 0);
  return 0;
}
