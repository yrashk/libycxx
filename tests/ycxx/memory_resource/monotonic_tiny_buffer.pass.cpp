// [mem.res.monotonic.buffer.ctor]/4: monotonic_buffer_resource(buffer, buffer_size, upstream)
// "Sets upstream_rsrc to upstream, current_buffer to buffer, and next_buffer_size to
// buffer_size (but not less than 1)" -- so buffer_size may be 0 or 1, and the buffer may start
// at any address. [mem.res.monotonic.buffer.mem]/4: "If the unused space in current_buffer can
// fit a block with the specified bytes and alignment, then allocate the return block from
// current_buffer; otherwise set current_buffer to upstream_rsrc->allocate(n, m), where n is
// not less than max(bytes, next_buffer_size) and m is not less than alignment". /1: release()
// "Resets current_buffer and next_buffer_size to their initial values at construction" (the
// user buffer is used again). /5: the result is aligned ([mem.res.private]/2) even when the
// user buffer is oddly aligned. /6: "Throws: Nothing unless upstream_rsrc->allocate()
// throws": a throwing upstream leaves the resource usable.
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <new>
#include "check.hpp"
#include "recording_resource.hpp"

using M = std::pmr::monotonic_buffer_resource;

static bool inside(void* p, std::size_t n, void* buf, std::size_t size) {
  auto a = reinterpret_cast<std::uintptr_t>(p), b = reinterpret_cast<std::uintptr_t>(buf);
  return a >= b && a + n <= b + size;
}

alignas(64) static unsigned char storage[256];

int main() {
  RecordingResource up;
  unsigned char* odd = storage + 1;  // address = 1 (mod 64)

  // buffer_size 1 at an odd address: exactly one byte with alignment 1 fits.
  {
    M m(odd, 1, &up);
    void* a = m.allocate(1, 1);
    CHECK(a == odd && up.allocs == 0);
    *static_cast<unsigned char*>(a) = 0x5a;
    void* b = m.allocate(1, 1);  // no room left: upstream
    CHECK(up.allocs == 1 && up.last_bytes >= 1 && up.last_align >= 1);
    CHECK(!inside(b, 1, odd, 1));
    void* c = m.allocate(1, 2);
    CHECK(reinterpret_cast<std::uintptr_t>(c) % 2 == 0);
    m.release();
    CHECK(up.outstanding == 0 && storage[1] == 0x5a);
    CHECK(m.allocate(1, 1) == odd);  // the initial buffer again
    CHECK(up.allocs == 1);
  }
  CHECK(up.outstanding == 0);

  // buffer_size 1 at an odd address and an alignment the byte cannot meet: upstream, with m
  // not less than the alignment.
  {
    M m(odd, 1, &up);
    int before = up.allocs;
    void* a = m.allocate(1, 2);
    CHECK(up.allocs == before + 1 && up.last_align >= 2 && up.last_bytes >= 1);
    CHECK(reinterpret_cast<std::uintptr_t>(a) % 2 == 0 && !inside(a, 1, odd, 1));
  }
  CHECK(up.outstanding == 0);

  // buffer_size 7 at address 1 (mod 8): the only 4-aligned 4-byte block inside is at +3.
  {
    M m(odd, 7, &up);
    int before = up.allocs;
    void* a = m.allocate(4, 4);
    CHECK(a == odd + 3 && up.allocs == before);
    void* b = m.allocate(4, 4);  // nothing left for it
    CHECK(up.allocs == before + 1 && !inside(b, 4, odd, 7));
  }
  CHECK(up.outstanding == 0);

  // buffer_size 0 (with a non-null and with a null buffer): the first request goes upstream.
  for (void* buf : {static_cast<void*>(odd), static_cast<void*>(nullptr)}) {
    M m(buf, 0, &up);
    int before = up.allocs;
    void* a = m.allocate(1, 1);
    CHECK(up.allocs == before + 1 && up.last_bytes >= 1);
    CHECK(a != nullptr);
    void* b = m.allocate(10, 8);
    CHECK(reinterpret_cast<std::uintptr_t>(b) % 8 == 0);
    m.release();
    CHECK(up.outstanding == 0);
    (void)m.allocate(1, 1);
    CHECK(up.allocs >= before + 1);
  }
  CHECK(up.outstanding == 0);

  // Over-aligned requests through a tiny odd buffer: upstream with m >= alignment.
  for (std::size_t al : {64u, 4096u, 8192u}) {
    M m(odd, 3, &up);
    void* a = m.allocate(5, al);
    CHECK(reinterpret_cast<std::uintptr_t>(a) % al == 0 && up.last_align >= al);
    void* b = m.allocate(5, al);  // from the current buffer or upstream: aligned either way
    CHECK(reinterpret_cast<std::uintptr_t>(b) % al == 0 && b != a);
  }
  CHECK(up.outstanding == 0);

  // Upstream throws in the middle of growth: bad_alloc propagates; earlier blocks keep their
  // contents; the resource works again once upstream does; release() frees everything.
  {
    M m(odd, 5, &up);
    unsigned char* blocks[40];
    for (int i = 0; i < 20; ++i) {
      blocks[i] = static_cast<unsigned char*>(m.allocate(100, 8));
      std::memset(blocks[i], i + 1, 100);
    }
    up.fail = true;
    bool threw = false;
    try {
      for (int i = 0; i < 1000; ++i) (void)m.allocate(1000, 8);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    CHECK(threw);
    up.fail = false;
    for (int i = 20; i < 40; ++i) {
      blocks[i] = static_cast<unsigned char*>(m.allocate(100, 8));
      std::memset(blocks[i], i + 1, 100);
    }
    for (int i = 0; i < 40; ++i)
      for (int k = 0; k < 100; ++k) CHECK(blocks[i][k] == i + 1);
    m.release();
    CHECK(up.outstanding == 0);
    CHECK(m.allocate(5, 1) == odd);
  }
  CHECK(up.outstanding == 0);
  return 0;
}
