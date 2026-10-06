// libycxx runtime (hosted and freestanding): the allocation table (allocation_table.hpp).
#include <new>
#include "allocation_table.hpp"

namespace {
using std::size_t;
using std::align_val_t;

// Each calls this image's `::operator ...`: the program's replacement where the program has one.
void* new_(size_t n, size_t) { return ::operator new(n); }
void* new_align(size_t n, size_t a) { return ::operator new(n, align_val_t(a)); }
void* new_nothrow(size_t n, size_t) noexcept { return ::operator new(n, std::nothrow); }
void* new_align_nothrow(size_t n, size_t a) noexcept { return ::operator new(n, align_val_t(a), std::nothrow); }
void* new_array(size_t n, size_t) { return ::operator new[](n); }
void* new_array_align(size_t n, size_t a) { return ::operator new[](n, align_val_t(a)); }
void* new_array_nothrow(size_t n, size_t) noexcept { return ::operator new[](n, std::nothrow); }
void* new_array_align_nothrow(size_t n, size_t a) noexcept {
  return ::operator new[](n, align_val_t(a), std::nothrow);
}
void delete_(void* p, size_t, size_t) noexcept { ::operator delete(p); }
void delete_sized(void* p, size_t n, size_t) noexcept { ::operator delete(p, n); }
void delete_align(void* p, size_t, size_t a) noexcept { ::operator delete(p, align_val_t(a)); }
void delete_sized_align(void* p, size_t n, size_t a) noexcept { ::operator delete(p, n, align_val_t(a)); }
void delete_nothrow(void* p, size_t, size_t) noexcept { ::operator delete(p, std::nothrow); }
void delete_align_nothrow(void* p, size_t, size_t a) noexcept { ::operator delete(p, align_val_t(a), std::nothrow); }
void delete_array(void* p, size_t, size_t) noexcept { ::operator delete[](p); }
void delete_array_sized(void* p, size_t n, size_t) noexcept { ::operator delete[](p, n); }
void delete_array_align(void* p, size_t, size_t a) noexcept { ::operator delete[](p, align_val_t(a)); }
void delete_array_sized_align(void* p, size_t n, size_t a) noexcept { ::operator delete[](p, n, align_val_t(a)); }
void delete_array_nothrow(void* p, size_t, size_t) noexcept { ::operator delete[](p, std::nothrow); }
void delete_array_align_nothrow(void* p, size_t, size_t a) noexcept {
  ::operator delete[](p, align_val_t(a), std::nothrow);
}

constexpr ycxx_allocation_functions_t entries = {
    new_,          new_align,          new_nothrow,          new_align_nothrow,
    new_array,     new_array_align,    new_array_nothrow,    new_array_align_nothrow,
    delete_,       delete_sized,       delete_align,         delete_sized_align,
    delete_nothrow, delete_align_nothrow, delete_array,      delete_array_sized,
    delete_array_align, delete_array_sized_align, delete_array_nothrow, delete_array_align_nothrow};
} // namespace

// Weak and exported: the dynamic linker binds every image's references to the first image's
// definition. The name is libycxx's alone, so no other C++ runtime takes part.
extern "C" [[gnu::weak, gnu::visibility("default")]] constinit const ycxx_allocation_functions_t
    ycxx_allocation_functions = entries;

// What the link options name as undefined (cmake/ycxx-link.cmake) to pull this member into a
// program: hidden, so a shared library on the program's link line, which exports
// ycxx_allocation_functions, cannot satisfy the reference instead (the program would then have no
// table of its own, and every image would use that library's).
extern "C" [[gnu::visibility("hidden")]] const char ycxx_allocation_table_anchor = 0;

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
constinit const ycxx_allocation_functions_t own_allocation_functions = entries;
}} // namespace ycxx::detail
