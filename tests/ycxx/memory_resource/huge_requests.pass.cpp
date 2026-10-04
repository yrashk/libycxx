// Requests near SIZE_MAX must not wrap around inside a resource:
// [mem.res.monotonic.buffer.mem]/4: when a request does not fit, the resource calls
// "upstream_rsrc->allocate(n, m), where n is not less than max(bytes, next_buffer_size) and m
// is not less than alignment"; /5: the result has "a size of at least bytes"; /6 "Throws:
// Nothing unless upstream_rsrc->allocate() throws". [mem.res.pool.mem]/4: "If bytes is larger
// than that which the largest pool can handle, then memory will be allocated using
// upstream_resource()->allocate()"; /5-6 likewise. So for bytes close to SIZE_MAX (with any
// alignment) the upstream request -- if one is made -- asks for at least bytes, and the
// failure of an upstream that cannot provide it propagates as bad_alloc; a resource never
// returns a block smaller than requested. [mem.res.pool.options]/2-3: max_blocks_per_chunk and
// largest_required_pool_block of SIZE_MAX are replaced by implementation limits, after which
// the pool works normally. [mem.res.monotonic.buffer.ctor]/2: an initial_size near SIZE_MAX is
// a valid request size for the first upstream call.
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <new>
#include "check.hpp"
#include "recording_resource.hpp"

// Records every request; refuses those above a limit with bad_alloc.
struct Refusing : std::pmr::memory_resource {
  std::size_t max_seen = 0, min_seen = std::numeric_limits<std::size_t>::max();
  std::size_t limit = 1 << 20;
  int refused = 0;
  long outstanding = 0;
  void* do_allocate(std::size_t bytes, std::size_t align) override {
    if (bytes > max_seen) max_seen = bytes;
    if (bytes < min_seen) min_seen = bytes;
    if (bytes > limit) {
      ++refused;
      throw std::bad_alloc();
    }
    outstanding += static_cast<long>(bytes);
    return ::operator new(bytes, std::align_val_t(align));
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t align) override {
    outstanding -= static_cast<long>(bytes);
    ::operator delete(p, bytes, std::align_val_t(align));
  }
  bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};

constexpr std::size_t MAX = std::numeric_limits<std::size_t>::max();

template <class R>
void huge(R& r, Refusing& up) {
  const std::size_t sizes[] = {MAX, MAX - 1, MAX - 7, MAX - 64, MAX - 4095, MAX / 2 + 1, MAX / 2};
  const std::size_t aligns[] = {1, 8, 64, 4096};
  for (std::size_t b : sizes)
    for (std::size_t al : aligns) {
      int refused = up.refused;
      bool threw = false;
      void* p = nullptr;
      try {
        p = r.allocate(b, al);
      } catch (const std::bad_alloc&) {
        threw = true;
      }
      CHECK(threw && p == nullptr);  // nobody can provide that much
      if (up.refused > refused) CHECK(up.max_seen >= b);  // an upstream request is at least bytes
    }
  // Still usable.
  void* p = r.allocate(100, 16);
  CHECK(p != nullptr && reinterpret_cast<std::uintptr_t>(p) % 16 == 0);
  r.deallocate(p, 100, 16);
}

int main() {
  {
    Refusing up;
    std::pmr::monotonic_buffer_resource m(&up);
    huge(m, up);
  }
  {
    Refusing up;
    std::pmr::unsynchronized_pool_resource u(&up);
    huge(u, up);
    u.release();
    CHECK(up.outstanding == 0);
  }
  {
    Refusing up;
    std::pmr::synchronized_pool_resource s(&up);
    huge(s, up);
  }
  {  // Implementation limits replace SIZE_MAX options.
    RecordingResource up;
    for (auto o : {std::pmr::pool_options{MAX, MAX}, std::pmr::pool_options{MAX, 1}, std::pmr::pool_options{1, MAX}}) {
      std::pmr::unsynchronized_pool_resource u(o, &up);
      std::pmr::pool_options got = u.options();
      CHECK(got.max_blocks_per_chunk > 0 && got.largest_required_pool_block > 0);
      void* ps[64];
      for (int i = 0; i < 64; ++i) {
        ps[i] = u.allocate(static_cast<std::size_t>(1) << (i % 12), 8);
        CHECK(ps[i] != nullptr);
      }
      for (int i = 0; i < 64; ++i) u.deallocate(ps[i], static_cast<std::size_t>(1) << (i % 12), 8);
    }
    CHECK(up.outstanding == 0);
  }
  {  // A huge initial_size is the size of the first upstream request.
    Refusing up;
    std::pmr::monotonic_buffer_resource m(MAX / 2, &up);
    bool threw = false;
    try {
      (void)m.allocate(1, 1);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    CHECK(threw && up.refused == 1 && up.max_seen >= MAX / 2);
  }
  {  // next_buffer_size growth never wraps to a request smaller than bytes.
    Refusing up;
    up.limit = MAX;  // grant everything up to what operator new can do
    std::pmr::monotonic_buffer_resource m(1 << 16, &up);
    for (int i = 0; i < 40; ++i) {
      (void)m.allocate(1 << 15, 8);
    }
    CHECK(up.min_seen >= (1 << 15));
  }
  return 0;
}
