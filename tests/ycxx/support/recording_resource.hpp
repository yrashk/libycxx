// A memory_resource for libycxx's own suite, written from [mem.res.class]: it forwards to
// operator new/delete (aligned) and records every do_allocate/do_deallocate call.
#pragma once
#include <cstddef>
#include <memory_resource>
#include <new>

struct RecordingResource : std::pmr::memory_resource {
  int allocs = 0, deallocs = 0;
  std::size_t last_bytes = 0, last_align = 0;
  long outstanding = 0;  // bytes allocated minus bytes deallocated
  bool fail = false;     // throw bad_alloc from do_allocate

 private:
  void* do_allocate(std::size_t bytes, std::size_t align) override {
    if (fail) throw std::bad_alloc();
    ++allocs;
    last_bytes = bytes;
    last_align = align;
    outstanding += static_cast<long>(bytes);
    return ::operator new(bytes, std::align_val_t(align));
  }
  void do_deallocate(void* p, std::size_t bytes, std::size_t align) override {
    ++deallocs;
    last_bytes = bytes;
    last_align = align;
    outstanding -= static_cast<long>(bytes);
    ::operator delete(p, bytes, std::align_val_t(align));
  }
  bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};
