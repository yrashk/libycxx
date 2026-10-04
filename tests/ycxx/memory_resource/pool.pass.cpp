// [mem.res.pool]: synchronized_pool_resource and unsynchronized_pool_resource: allocate blocks
// (properly aligned, distinct while live), reuse deallocated blocks, obtain memory from the
// upstream resource given at construction (upstream_resource()), satisfy requests larger than
// the largest pool block directly from upstream, free everything on release() and on
// destruction (even blocks not deallocated). options() reports the (possibly adjusted, never
// zero) pool options. do_is_equal is identity. Not copyable.
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <set>
#include <type_traits>
#include "check.hpp"
#include "recording_resource.hpp"

static_assert(!std::is_copy_constructible_v<std::pmr::synchronized_pool_resource>);
static_assert(!std::is_copy_constructible_v<std::pmr::unsynchronized_pool_resource>);
static_assert(std::is_aggregate_v<std::pmr::pool_options>);

template <class Pool>
void exercise() {
  RecordingResource up;
  {
    std::pmr::pool_options opts;
    CHECK(opts.max_blocks_per_chunk == 0 && opts.largest_required_pool_block == 0);
    opts.largest_required_pool_block = 256;
    Pool pool(opts, &up);
    CHECK(pool.upstream_resource() == &up);
    std::pmr::pool_options got = pool.options();
    CHECK(got.max_blocks_per_chunk > 0 && got.largest_required_pool_block >= 256);

    std::set<void*> live;
    for (int i = 0; i < 50; ++i) {
      void* p = pool.allocate(24, 8);
      CHECK(reinterpret_cast<std::uintptr_t>(p) % 8 == 0);
      CHECK(live.insert(p).second);
    }
    void* a16 = pool.allocate(48, 16);
    CHECK(reinterpret_cast<std::uintptr_t>(a16) % 16 == 0);
    int allocs = up.allocs;
    CHECK(allocs > 0 && allocs < 51);
    for (void* p : live) pool.deallocate(p, 24, 8);
    // Deallocated blocks are reused: no further upstream allocation for the same pattern.
    for (int i = 0; i < 50; ++i) (void)pool.allocate(24, 8);
    CHECK(up.allocs == allocs);

    // An allocation beyond the largest pool block goes straight upstream.
    std::size_t huge = got.largest_required_pool_block * 4 + 1024;
    void* h = pool.allocate(huge, 32);
    CHECK(up.allocs > allocs && reinterpret_cast<std::uintptr_t>(h) % 32 == 0);
    pool.deallocate(h, huge, 32);

    pool.release();
    CHECK(up.outstanding == 0);
    void* p = pool.allocate(8);  // usable after release
    CHECK(p != nullptr);
    CHECK(pool == pool && !(pool == up));
  }
  CHECK(up.outstanding == 0);  // destructor released the block allocated after release()

  std::pmr::set_default_resource(&up);
  {
    Pool d;
    CHECK(d.upstream_resource() == &up);
    Pool o(std::pmr::pool_options{4, 64});
    CHECK(o.upstream_resource() == &up);
    Pool e(&up);
    (void)e.allocate(10);
  }
  std::pmr::set_default_resource(nullptr);
  CHECK(up.outstanding == 0);
}

int main() {
  exercise<std::pmr::unsynchronized_pool_resource>();
  exercise<std::pmr::synchronized_pool_resource>();
  return 0;
}
