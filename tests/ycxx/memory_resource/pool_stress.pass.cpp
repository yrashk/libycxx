// [mem.res.pool.mem]/4-5: do_allocate returns "a pointer to allocated storage
// ([basic.stc.dynamic.allocation]) with a size of at least bytes. The size and alignment of
// the allocated memory shall meet the requirements for a class derived from memory_resource",
// i.e. [mem.res.private]/2: "aligned to the specified alignment, if such alignment is
// supported" (operator new(size_t, align_val_t) supports every power of two used here); blocks
// live at the same time do not overlap ([basic.stc.dynamic.allocation]/2). This holds for
// every size, including requests larger than options().largest_required_pool_block (served by
// upstream, /4) and over-aligned requests of every size. /7: do_deallocate returns memory to
// the pool; /1: release() frees everything, after which the resource is used again.
// /6: "Throws: Nothing unless upstream_resource()->allocate() throws": when upstream throws
// bad_alloc in the middle of growth, the exception propagates and the resource stays usable
// (blocks obtained before keep their contents, later requests succeed once upstream recovers).
// /9: do_is_equal is identity, also between two pools sharing an upstream.
// REQUIRES: exceptions
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>
#include "check.hpp"
#include "recording_resource.hpp"

struct Block {
  unsigned char* p;
  std::size_t bytes, align;
  unsigned char tag;
};

static void fill(const Block& b) { std::memset(b.p, b.tag, b.bytes); }
static bool intact(const Block& b) {
  for (std::size_t i = 0; i < b.bytes; ++i)
    if (b.p[i] != b.tag) return false;
  return true;
}

static unsigned rng_state = 12345;
static unsigned rng() { return rng_state = rng_state * 1103515245u + 12345u, (rng_state >> 8) & 0xffffff; }

template <class Pool>
void stress(std::pmr::pool_options opts) {
  RecordingResource up;
  {
    Pool pool(opts, &up);
    const std::size_t largest = pool.options().largest_required_pool_block;
    CHECK(largest > 0);
    std::vector<Block> live;
    const std::size_t aligns[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 4096};
    unsigned char tag = 1;
    for (int round = 0; round < 3; ++round) {
      for (int i = 0; i < 600; ++i) {
        std::size_t bytes;
        switch (rng() % 4) {
          case 0: bytes = 1 + rng() % 16; break;
          case 1: bytes = 1 + rng() % (largest < 4096 ? largest : 4096); break;
          case 2: bytes = largest - 1 + rng() % 3; break;   // around the threshold
          default: bytes = largest + 1 + rng() % (2 * largest + 1); break;  // beyond it
        }
        if (bytes == 0) bytes = 1;
        std::size_t align = aligns[rng() % 10];
        void* p = pool.allocate(bytes, align);
        CHECK(p != nullptr);
        CHECK(reinterpret_cast<std::uintptr_t>(p) % align == 0);
        Block b{static_cast<unsigned char*>(p), bytes, align, tag};
        tag = static_cast<unsigned char>(tag % 250 + 1);
        fill(b);
        live.push_back(b);
        if (rng() % 3 == 0) {  // free a random live block
          std::size_t k = rng() % live.size();
          CHECK(intact(live[k]));
          pool.deallocate(live[k].p, live[k].bytes, live[k].align);
          live[k] = live.back();
          live.pop_back();
        }
      }
      for (const Block& b : live) CHECK(intact(b));  // no overlap between live blocks
      if (round == 1) {
        for (const Block& b : live) pool.deallocate(b.p, b.bytes, b.align);
      } else {
        pool.release();
        CHECK(up.outstanding == 0);
      }
      live.clear();
    }
  }
  CHECK(up.outstanding == 0);
}

// Upstream fails part-way: bad_alloc propagates, earlier blocks stay intact, the pool works
// again when upstream does.
template <class Pool>
void upstream_failure() {
  RecordingResource up;
  Pool pool(std::pmr::pool_options{0, 512}, &up);
  std::vector<Block> live;
  const std::size_t sizes[] = {8, 24, 100, 512, 4000};
  unsigned char tag = 1;
  for (std::size_t s : sizes) {
    int threw = 0;
    for (int i = 0; i < 2000 && !threw; ++i) {
      if (i == 5) up.fail = true;
      try {
        void* p = pool.allocate(s, 8);
        Block b{static_cast<unsigned char*>(p), s, 8, tag};
        tag = static_cast<unsigned char>(tag % 250 + 1);
        fill(b);
        live.push_back(b);
      } catch (const std::bad_alloc&) {
        threw = 1;
      }
    }
    CHECK(threw);  // a pool cannot grow without its upstream
    up.fail = false;
    for (const Block& b : live) CHECK(intact(b));
    for (int i = 0; i < 50; ++i) {
      void* p = pool.allocate(s, 8);
      Block b{static_cast<unsigned char*>(p), s, 8, tag};
      tag = static_cast<unsigned char>(tag % 250 + 1);
      fill(b);
      live.push_back(b);
    }
    for (const Block& b : live) CHECK(intact(b));
  }
  for (const Block& b : live) pool.deallocate(b.p, b.bytes, b.align);
  pool.release();
  CHECK(up.outstanding == 0);
}

template <class Pool>
void equality() {
  RecordingResource up;
  Pool a(&up), b(&up);
  std::pmr::memory_resource& ra = a;
  std::pmr::memory_resource& rb = b;
  CHECK(ra.is_equal(ra));
  CHECK(!ra.is_equal(rb) && !rb.is_equal(ra));
  CHECK(!(ra == rb) && ra != rb);
  CHECK(!ra.is_equal(up) && !up.is_equal(ra));
  std::pmr::polymorphic_allocator<int> pa(&a), pb(&b), pa2(&a);
  CHECK(pa == pa2 && pa != pb);
}

int main() {
  using U = std::pmr::unsynchronized_pool_resource;
  using S = std::pmr::synchronized_pool_resource;
  stress<U>({0, 0});
  stress<U>({1, 8});
  stress<U>({3, 100});
  stress<U>({0, 1000});
  stress<S>({0, 0});
  stress<S>({2, 64});
  upstream_failure<U>();
  upstream_failure<S>();
  equality<U>();
  equality<S>();
  return 0;
}
