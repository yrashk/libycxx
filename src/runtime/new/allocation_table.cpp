// libycxx runtime (hosted and freestanding): the allocation table (allocation_table.hpp).
#include <new>
#include "allocation_table.hpp"

namespace {
using std::size_t;
using std::align_val_t;

// Each calls this image's `::operator ...`: the program's replacement where the program has one.
void* __new_(size_t n, size_t) { return ::operator new(n); }
void* __new_align(size_t n, size_t a) { return ::operator new(n, align_val_t(a)); }
void* __new_nothrow(size_t n, size_t) noexcept { return ::operator new(n, std::nothrow); }
void* __new_align_nothrow(size_t n, size_t a) noexcept { return ::operator new(n, align_val_t(a), std::nothrow); }
void* __new_array(size_t n, size_t) { return ::operator new[](n); }
void* __new_array_align(size_t n, size_t a) { return ::operator new[](n, align_val_t(a)); }
void* __new_array_nothrow(size_t n, size_t) noexcept { return ::operator new[](n, std::nothrow); }
void* __new_array_align_nothrow(size_t n, size_t a) noexcept {
  return ::operator new[](n, align_val_t(a), std::nothrow);
}
void __delete_(void* p, size_t, size_t) noexcept { ::operator delete(p); }
void __delete_sized(void* p, size_t n, size_t) noexcept { ::operator delete(p, n); }
void __delete_align(void* p, size_t, size_t a) noexcept { ::operator delete(p, align_val_t(a)); }
void __delete_sized_align(void* p, size_t n, size_t a) noexcept { ::operator delete(p, n, align_val_t(a)); }
void __delete_nothrow(void* p, size_t, size_t) noexcept { ::operator delete(p, std::nothrow); }
void __delete_align_nothrow(void* p, size_t, size_t a) noexcept { ::operator delete(p, align_val_t(a), std::nothrow); }
void __delete_array(void* p, size_t, size_t) noexcept { ::operator delete[](p); }
void __delete_array_sized(void* p, size_t n, size_t) noexcept { ::operator delete[](p, n); }
void __delete_array_align(void* p, size_t, size_t a) noexcept { ::operator delete[](p, align_val_t(a)); }
void __delete_array_sized_align(void* p, size_t n, size_t a) noexcept { ::operator delete[](p, n, align_val_t(a)); }
void __delete_array_nothrow(void* p, size_t, size_t) noexcept { ::operator delete[](p, std::nothrow); }
void __delete_array_align_nothrow(void* p, size_t, size_t a) noexcept {
  ::operator delete[](p, align_val_t(a), std::nothrow);
}

constexpr __ycxx_allocation_functions_t __entries = {
    __new_,          __new_align,          __new_nothrow,          __new_align_nothrow,
    __new_array,     __new_array_align,    __new_array_nothrow,    __new_array_align_nothrow,
    __delete_,       __delete_sized,       __delete_align,         __delete_sized_align,
    __delete_nothrow, __delete_align_nothrow, __delete_array,      __delete_array_sized,
    __delete_array_align, __delete_array_sized_align, __delete_array_nothrow, __delete_array_align_nothrow};
} // namespace

// Weak and exported: the dynamic linker binds every image's references to the first image's
// definition. The name is libycxx's alone, so no other C++ runtime takes part.
extern "C" [[__gnu__::__weak__, __gnu__::__visibility__("default")]] constinit const __ycxx_allocation_functions_t
    __ycxx_allocation_functions = __entries;

// What the link options name as undefined (cmake/ycxx-link.cmake) to pull this member into a
// program: hidden, so a shared library on the program's link line, which exports
// __ycxx_allocation_functions, cannot satisfy the reference instead (the program would then have no
// table of its own, and every image would use that library's).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_table_anchor = 0;

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
constinit const __ycxx_allocation_functions_t __own_allocation_functions = __entries;
}} // namespace __ycxx::__detail
