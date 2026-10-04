// Replacement global allocation functions ([new.delete], [replacement.functions]) for the
// exception-injection harness: they count live blocks (exh::new_live) and, when the gnew kind
// is armed, the k-th call fails: the throwing forms throw exh::alloc_failure (a bad_alloc,
// [new.delete.single]/3), the nothrow forms return null ([new.delete.single]/8).
// Include from exactly one translation unit of a test (replacement functions are not inline).
#pragma once
#include <cstdlib>
#include <new>
#include "exc_harness.hpp"

namespace exh {
inline long new_live = 0;
inline void* new_impl(std::size_t n, std::size_t align, bool nothrow) {
  if (should_throw(gnew)) {
    if (nothrow) return nullptr;
    throw alloc_failure(gnew);
  }
  if (n == 0) n = 1;
  void* p;
  if (align <= alignof(std::max_align_t))
    p = std::malloc(n);
  else
    p = std::aligned_alloc(align, (n + align - 1) / align * align);
  if (!p) {
    if (nothrow) return nullptr;
    throw std::bad_alloc();
  }
  ++new_live;
  return p;
}
inline void delete_impl(void* p) {
  if (!p) return;
  --new_live;
  std::free(p);
}
} // namespace exh

void* operator new(std::size_t n) { return exh::new_impl(n, 0, false); }
void* operator new[](std::size_t n) { return exh::new_impl(n, 0, false); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return exh::new_impl(n, 0, true); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return exh::new_impl(n, 0, true); }
void* operator new(std::size_t n, std::align_val_t a) { return exh::new_impl(n, std::size_t(a), false); }
void* operator new[](std::size_t n, std::align_val_t a) { return exh::new_impl(n, std::size_t(a), false); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  return exh::new_impl(n, std::size_t(a), true);
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  return exh::new_impl(n, std::size_t(a), true);
}
void operator delete(void* p) noexcept { exh::delete_impl(p); }
void operator delete[](void* p) noexcept { exh::delete_impl(p); }
void operator delete(void* p, std::size_t) noexcept { exh::delete_impl(p); }
void operator delete[](void* p, std::size_t) noexcept { exh::delete_impl(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { exh::delete_impl(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { exh::delete_impl(p); }
void operator delete(void* p, std::align_val_t) noexcept { exh::delete_impl(p); }
void operator delete[](void* p, std::align_val_t) noexcept { exh::delete_impl(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { exh::delete_impl(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { exh::delete_impl(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { exh::delete_impl(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { exh::delete_impl(p); }

namespace exh {
// A sweep over the gnew kind that also checks that operator new blocks are balanced.
template <class F>
void sweep_new(const char* name, F&& scenario, options o = {}) {
  long base = 0;
  sweep(name, gnew, [&] {
    base = new_live;
    bool threw = scenario();
    if (new_live != base) {
      char buf[96];
      __builtin_snprintf(buf, sizeof buf, "%ld operator new block(s) not freed", new_live - base);
      basic_tracked<false>::report_dyn(buf);
      new_live = base;
    }
    return threw;
  }, o);
}
// Checks operator new balance around any sweep (for other kinds).
template <class F>
auto new_balanced(F&& scenario) {
  return [scenario] {
    long base = new_live;
    bool threw = scenario();
    if (new_live != base) {
      char buf[96];
      __builtin_snprintf(buf, sizeof buf, "%ld operator new block(s) not freed", new_live - base);
      basic_tracked<false>::report_dyn(buf);
      new_live = base;
    }
    return threw;
  };
}
} // namespace exh
