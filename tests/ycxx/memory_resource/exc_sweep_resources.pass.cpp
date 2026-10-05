// Exception-injection sweep over the standard memory resources and pmr containers: an upstream
// memory_resource whose do_allocate throws at its k-th call (and records every block with its
// size and alignment), for every k until the scenario completes.
//   [mem.res.pool.mem]/4-6, [mem.res.monotonic.buffer.mem]/4-6: do_allocate "Throws: Nothing
//     unless upstream_resource()->allocate() throws"; afterwards the resource is still usable
//     (subsequent allocations and deallocations work) and [mem.res.pool.mem]/1,
//     [mem.res.monotonic.buffer.mem]/1, [mem.res.pool.ctor]/4: release() and the destructor
//     return all memory to upstream: every upstream block is deallocated exactly once, with the
//     same size and alignment it was allocated with ([mem.res.public]/4: deallocate(p, bytes,
//     alignment): "p was returned from a prior call to allocate(bytes, alignment)").
//   Allocations returned are aligned as requested ([mem.res.private]/2).
//   pmr containers ([mem.poly.allocator.class]; [allocator.uses.construction] for the nested
//   strings and vectors) over the throwing resource: every element destroyed exactly once and
//   every block returned with matching size and alignment, also when copying between
//   containers with different resources (unequal allocators: element-wise, uses-allocator
//   construction with the target's resource).
// REQUIRES: exceptions
#include <cstdint>
#include <map>
#include <memory>
#include <memory_resource>
#include <string>
#include <unordered_map>
#include <vector>
#include "exc_harness.hpp"

using namespace exh;

struct Upstream : std::pmr::memory_resource {
  long live = 0;
  void* do_allocate(std::size_t bytes, std::size_t align) override {
    alloc_point(allocation);
    ++live;
    return registry_alloc(bytes, 1, align < 16 ? 16 : align, int(align)); // (the id records the alignment)
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t align) override {
    --live;
    // the size must match; the alignment is checked through the id
    registry_dealloc(p, bytes, 1, int(align));
  }
  bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
};

static bool aligned(void* p, std::size_t a) { return reinterpret_cast<std::uintptr_t>(p) % a == 0; }

// A fixed workload of allocations and deallocations on r.
static void workload(std::pmr::memory_resource& r, int seed) {
  struct A {
    void* p;
    std::size_t n, a;
  } live[64];
  int nl = 0;
  static const std::size_t sizes[] = {1, 8, 24, 100, 256, 1000, 5000, 40000, 3, 64};
  static const std::size_t aligns[] = {1, 2, 8, 16, 64, 256, 4096};
  for (int i = 0; i < 120; ++i) {
    int x = (i * 7 + seed * 13) % 23;
    if (x < 15 || nl == 0) {
      std::size_t n = sizes[(i + seed) % 10], a = aligns[(i * 3 + seed) % 7];
      void* p = r.allocate(n, a);
      if (!aligned(p, a)) report("allocation not aligned as requested", __LINE__);
      for (std::size_t j = 0; j < n; j += 97) static_cast<unsigned char*>(p)[j] = 0x5a;
      live[nl++] = A{p, n, a};
      if (nl == 64) {
        r.deallocate(live[nl - 1].p, live[nl - 1].n, live[nl - 1].a);
        --nl;
      }
    } else {
      int j = x % nl;
      r.deallocate(live[j].p, live[j].n, live[j].a);
      live[j] = live[--nl];
    }
  }
  while (nl > 0) {
    --nl;
    r.deallocate(live[nl].p, live[nl].n, live[nl].a);
  }
}

template <class Make>
void resource_sweep(const char* name, Make make) {
  sweep(name, allocation, [&] {
    Upstream up;
    int nb0 = nblocks;
    bool threw;
    {
      auto r = make(&up);
      threw = attempt([&] { workload(*r, 1); });
      // still usable after an upstream failure
      workload(*r, 2);
      r->release();
      if (up.live != 0) report("release() did not return every upstream block", __LINE__);
      workload(*r, 3);
    }
    EXH_EXPECT(up.live == 0, "the destructor did not return every upstream block");
    EXH_EXPECT(nblocks == nb0, "upstream blocks leaked");
    return threw;
  });
}

int main() {
  resource_sweep("unsynchronized_pool_resource()", [](Upstream* u) {
    return std::make_unique<std::pmr::unsynchronized_pool_resource>(u);
  });
  resource_sweep("unsynchronized_pool_resource({4, 512})", [](Upstream* u) {
    return std::make_unique<std::pmr::unsynchronized_pool_resource>(std::pmr::pool_options{4, 512}, u);
  });
  resource_sweep("synchronized_pool_resource({0, 64})", [](Upstream* u) {
    return std::make_unique<std::pmr::synchronized_pool_resource>(std::pmr::pool_options{0, 64}, u);
  });
  resource_sweep("monotonic_buffer_resource()", [](Upstream* u) {
    return std::make_unique<std::pmr::monotonic_buffer_resource>(u);
  });
  resource_sweep("monotonic_buffer_resource(1)", [](Upstream* u) {
    return std::make_unique<std::pmr::monotonic_buffer_resource>(1, u);
  });
  static unsigned char initial[300];
  resource_sweep("monotonic_buffer_resource(buffer, 300)", [](Upstream* u) {
    return std::make_unique<std::pmr::monotonic_buffer_resource>(initial, sizeof initial, u);
  });

  // pmr containers directly over the throwing resource
  using PS = std::pmr::string;
  static const char* words[] = {"a string longer than any small-buffer optimization would hold, #1",
                                "short", "another string longer than any small-buffer optimization, #3",
                                "x", "yet another fairly long string to force a separate allocation, #5"};
  auto pmr_sweep = [](const char* name, auto op) {
    sweep(name, allocation, [&] {
      Upstream up, other;
      int nb0 = nblocks;
      long live0 = st.live;
      bool threw = attempt([&] { op(up, other); });
      EXH_EXPECT(up.live == 0 && other.live == 0, "pmr container blocks not returned");
      EXH_EXPECT(nblocks == nb0, "blocks leaked");
      EXH_EXPECT(st.live == live0, "element objects leaked");
      return threw;
    });
  };
  pmr_sweep("pmr::vector<pmr::string>: emplace_back, insert, copy to another resource", [](Upstream& up, Upstream& other) {
    std::pmr::vector<PS> v(&up);
    for (int i = 0; i < 12; ++i) v.emplace_back(words[i % 5]);
    v.insert(v.begin() + 3, PS(words[0], &other));
    std::pmr::vector<PS> w(v, &other);
    w = v;
    v = std::move(w); // unequal resources: element-wise
  });
  pmr_sweep("pmr::map<pmr::string, pmr::vector<int>>", [](Upstream& up, Upstream& other) {
    std::pmr::map<PS, std::pmr::vector<int>> m(&up);
    for (int i = 0; i < 10; ++i) m[PS(words[i % 5]) + char('0' + i)].assign(20, i);
    std::pmr::map<PS, std::pmr::vector<int>> n(m, &other);
    n = m;
  });
  pmr_sweep("pmr::unordered_map<pmr::string, pmr::string>", [](Upstream& up, Upstream& other) {
    std::pmr::unordered_map<PS, PS> m(&up);
    for (int i = 0; i < 30; ++i) m.try_emplace(PS(words[i % 5]) + char('a' + i), words[(i + 1) % 5]);
    std::pmr::unordered_map<PS, PS> n(m, &other);
    m.rehash(200);
  });
  pmr_sweep("pmr::vector<T> over a pool over the throwing resource", [](Upstream& up, Upstream&) {
    std::pmr::unsynchronized_pool_resource pool(&up);
    std::pmr::vector<T> v(&pool);
    for (int i = 0; i < 40; ++i) v.emplace_back(i);
    std::pmr::vector<std::pmr::vector<T>> vv(&pool);
    for (int i = 0; i < 6; ++i) vv.emplace_back(v.begin(), v.begin() + i * 5);
  });
  return finish();
}
