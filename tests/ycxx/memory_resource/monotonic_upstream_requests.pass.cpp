// [mem.res.monotonic.buffer.ctor]/2: monotonic_buffer_resource(upstream) and
// (initial_size, upstream) set current_buffer to nullptr (so construction requests nothing
// from upstream) and next_buffer_size "to at least initial_size". /4: (buffer, buffer_size,
// upstream) sets next_buffer_size to buffer_size "then increases next_buffer_size by an
// implementation-defined growth factor". [mem.res.monotonic.buffer.mem]/4: when the request
// does not fit, "set current_buffer to upstream_rsrc->allocate(n, m), where n is not less
// than max(bytes, next_buffer_size) and m is not less than alignment, and increase
// next_buffer_size ..., then allocate the return block from the newly-allocated
// current_buffer"; otherwise the block comes from current_buffer (no upstream call). /5: the
// result is suitably aligned. /6: "Throws: Nothing unless upstream_rsrc->allocate() throws"
// (so with null_memory_resource() upstream, requests that fit the initial buffer succeed and
// the first one that does not throws bad_alloc). /1: release() resets next_buffer_size to its
// initial value, so the next upstream request is again at least initial_size; /7 do_deallocate
// has no effect, so deallocating does not make room.
// REQUIRES: exceptions
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <new>
#include "recording_resource.hpp"
#include "check.hpp"

static bool aligned(void* p, std::size_t a) { return reinterpret_cast<std::uintptr_t>(p) % a == 0; }

int main() {
  {  // no upstream request at construction
    RecordingResource up;
    std::pmr::monotonic_buffer_resource a(&up);
    std::pmr::monotonic_buffer_resource b(4096, &up);
    alignas(16) static unsigned char buf[64];
    std::pmr::monotonic_buffer_resource c(buf, sizeof buf, &up);
    CHECK(up.allocs == 0);
  }
  {  // initial_size bounds the first request; later small requests come from that buffer
    RecordingResource up;
    std::pmr::monotonic_buffer_resource m(4096, &up);
    void* p = m.allocate(1, 1);
    CHECK(up.allocs == 1);
    CHECK(up.last_bytes >= 4096 && up.last_align >= 1);
    const std::size_t first = up.last_bytes;
    for (std::size_t used = 1; used < first / 2; ++used) m.allocate(1, 1);
    CHECK(up.allocs == 1);  // all fit in the first buffer
    m.deallocate(p, 1, 1);  // no effect
    CHECK(up.deallocs == 0);
  }
  {  // n >= bytes and m >= alignment for a request larger than next_buffer_size
    RecordingResource up;
    std::pmr::monotonic_buffer_resource m(64, &up);
    void* p = m.allocate(10000, 256);
    CHECK(up.allocs == 1 && up.last_bytes >= 10000 && up.last_align >= 256);
    CHECK(aligned(p, 256));
    void* q = m.allocate(3, 64);
    CHECK(aligned(q, 64));
  }
  {  // the next_buffer_size never shrinks below initial_size; release() resets it
    RecordingResource up;
    std::pmr::monotonic_buffer_resource m(1000, &up);
    std::size_t total = 0;
    for (int i = 0; i < 200; ++i) {
      int before = up.allocs;
      m.allocate(100, 8);
      if (up.allocs != before) CHECK(up.last_bytes >= 1000);
      total += 100;
    }
    // Each buffer has at least 1000 bytes, room for at least 9 blocks of 100 bytes aligned to 8,
    // so 200 blocks need at most ceil(200 / 9) == 23 upstream requests.
    CHECK(up.allocs >= 1 && up.allocs <= 23);
    m.release();
    CHECK(up.outstanding == 0);
    m.allocate(1, 1);
    CHECK(up.last_bytes >= 1000);
    (void)total;
  }
  {  // initial buffer: next_buffer_size starts at buffer_size, so the first upstream request
     // after the buffer is used up is at least buffer_size bytes
    RecordingResource up;
    alignas(16) static unsigned char buf[512];
    std::pmr::monotonic_buffer_resource m(buf, sizeof buf, &up);
    unsigned char* p = static_cast<unsigned char*>(m.allocate(400, 1));
    CHECK(p >= buf && p + 400 <= buf + sizeof buf && up.allocs == 0);
    m.allocate(200, 1);  // does not fit
    CHECK(up.allocs == 1 && up.last_bytes >= 512);
  }
  {  // Throws: nothing unless upstream throws
    alignas(16) static unsigned char buf[256];
    std::pmr::monotonic_buffer_resource m(buf, sizeof buf, std::pmr::null_memory_resource());
    for (int i = 0; i < 8; ++i) m.allocate(16, 16);  // 128 bytes: fits
    bool threw = false;
    try {
      m.allocate(1000, 1);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    CHECK(threw);
    m.release();
    void* again = m.allocate(200, 1);  // the initial buffer is available again
    CHECK(static_cast<unsigned char*>(again) >= buf && static_cast<unsigned char*>(again) < buf + sizeof buf);
  }
  return 0;
}
