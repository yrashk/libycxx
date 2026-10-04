// [mem.res.pool.overview]/2: "A synchronized_pool_resource may be accessed from multiple
// threads without external synchronization". [mem.res.pool.mem]/5: do_allocate returns
// storage of at least bytes ([basic.stc.dynamic.allocation]: distinct while live, suitably
// aligned); /7 do_deallocate returns it to the pool. Several threads allocate, fill, verify
// and deallocate blocks of many sizes (pooled and larger than any pool, which go upstream,
// [mem.res.pool.overview]/1.4) concurrently; every live block keeps its contents, so no two
// live blocks overlap. Afterwards release() / destruction return everything upstream
// ([mem.res.pool.mem]/1, [mem.res.pool.ctor]/4).
#include <memory_resource>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include "check.hpp"

// A thread-safe counting upstream (new_delete_resource underneath).
struct CountingUpstream : std::pmr::memory_resource {
  std::atomic<long> outstanding{0};
  std::atomic<int> allocs{0};

 private:
  void* do_allocate(std::size_t bytes, std::size_t align) override {
    ++allocs;
    outstanding += static_cast<long>(bytes);
    return std::pmr::new_delete_resource()->allocate(bytes, align);
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t align) override {
    outstanding -= static_cast<long>(bytes);
    std::pmr::new_delete_resource()->deallocate(p, bytes, align);
  }
  bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
};

struct Block {
  unsigned char* p;
  std::size_t size;
  std::size_t align;
  unsigned char tag;
};

static bool intact(const Block& b) {
  for (std::size_t i = 0; i < b.size; ++i)
    if (b.p[i] != b.tag) return false;
  return true;
}

int main() {
  CountingUpstream up;
  std::atomic<int> failures{0};
  {
    std::pmr::synchronized_pool_resource pool(std::pmr::pool_options{16, 512}, &up);
    auto work = [&](int t) {
      std::vector<Block> live;
      std::uint32_t seed = 12345u + static_cast<std::uint32_t>(t) * 7919u;
      auto next = [&] {
        seed = seed * 1664525u + 1013904223u;
        return seed >> 8;
      };
      for (int i = 0; i < 4000; ++i) {
        if (live.size() < 64 && (next() % 3 != 0 || live.empty())) {
          static const std::size_t sizes[] = {1, 8, 13, 32, 64, 100, 256, 500, 1000, 4096};
          static const std::size_t aligns[] = {1, 2, 4, 8, 16, 32};
          std::size_t sz = sizes[next() % 10];
          std::size_t al = aligns[next() % 6];
          auto* p = static_cast<unsigned char*>(pool.allocate(sz, al));
          if (reinterpret_cast<std::uintptr_t>(p) % al != 0) ++failures;
          unsigned char tag = static_cast<unsigned char>(next());
          std::memset(p, tag, sz);
          live.push_back({p, sz, al, tag});
        } else {
          std::size_t k = next() % live.size();
          if (!intact(live[k])) ++failures;
          pool.deallocate(live[k].p, live[k].size, live[k].align);
          live[k] = live.back();
          live.pop_back();
        }
      }
      for (const Block& b : live) {
        if (!intact(b)) ++failures;
        pool.deallocate(b.p, b.size, b.align);
      }
    };
    std::vector<std::thread> threads;
    for (int t = 0; t < 6; ++t) threads.emplace_back(work, t);
    for (auto& th : threads) th.join();
    CHECK(failures == 0);
    CHECK(up.allocs > 0);
    // Blocks still allocated at release() are freed too.
    void* leaked = pool.allocate(24, 8);
    (void)leaked;
    pool.release();
    CHECK(up.outstanding == 0);
    void* after = pool.allocate(24, 8);  // usable after release
    pool.deallocate(after, 24, 8);
  }
  CHECK(up.outstanding == 0);
  return 0;
}
