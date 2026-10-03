// libycxx hosted runtime: replaceable global allocation functions, routed through the PAL.
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>

namespace {

void* allocate(std::size_t size, std::size_t align) {
  if (size == 0)
    size = 1;
  for (;;) {
    if (void* p = ycxx_pal_allocate(size, align))
      return p;
    std::new_handler h = std::get_new_handler();
    if (!h)
      ycxx::detail::throw_bad_alloc();
    h();
  }
}

void* allocate_nothrow(std::size_t size, std::size_t align) noexcept {
  if constexpr (ycxx::detail::cfg::exceptions) {
    try {
      return allocate(size, align);
    } catch (...) {
      return nullptr;
    }
  } else {
    if (size == 0)
      size = 1;
    return ycxx_pal_allocate(size, align);
  }
}

constexpr std::size_t default_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

} // namespace

void* operator new(std::size_t n) { return allocate(n, default_align); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, static_cast<std::size_t>(a)); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return allocate_nothrow(n, default_align); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  return allocate_nothrow(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void* operator new[](std::size_t n, std::align_val_t a) { return ::operator new(n, a); }
void* operator new[](std::size_t n, const std::nothrow_t& t) noexcept { return ::operator new(n, t); }
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t& t) noexcept {
  return ::operator new(n, a, t);
}

void operator delete(void* p) noexcept { ycxx_pal_deallocate(p, 0, default_align); }
void operator delete(void* p, std::size_t n) noexcept { ycxx_pal_deallocate(p, n, default_align); }
void operator delete(void* p, std::align_val_t a) noexcept { ycxx_pal_deallocate(p, 0, static_cast<std::size_t>(a)); }
void operator delete(void* p, std::size_t n, std::align_val_t a) noexcept {
  ycxx_pal_deallocate(p, n, static_cast<std::size_t>(a));
}
void operator delete(void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }
void operator delete(void* p, std::align_val_t a, const std::nothrow_t&) noexcept { ::operator delete(p, a); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t n) noexcept { ::operator delete(p, n); }
void operator delete[](void* p, std::align_val_t a) noexcept { ::operator delete(p, a); }
void operator delete[](void* p, std::size_t n, std::align_val_t a) noexcept { ::operator delete(p, n, a); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::align_val_t a, const std::nothrow_t&) noexcept { ::operator delete(p, a); }
