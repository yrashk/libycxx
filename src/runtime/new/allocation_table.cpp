// libycxx runtime (hosted and freestanding): the allocation table (allocation_table.hpp).
#include <new>
#include <ycxx/pal.h>
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
    __ycxx::__detail::__cfg::__abi_version,
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

// The ABI version check (DECISIONS §20.6, §20.10 step 8): every image linking libycxx has this
// initializer, which runs when the image is loaded. The process's table is the first image's; when
// it was built by another libycxx version (a plugin built against 0.2 loaded by a program built
// against 0.1, a static program and a shared-library plugin of different versions), the image
// stops before its code can exchange objects whose layouts disagree.
namespace {
void __append(char*& __out, const char* __end, const char* __s) noexcept {
  for (; *__s != 0 && __out != __end; ++__s)
    *__out++ = *__s;
}

[[__gnu__::__constructor__]] void __check_abi_version() noexcept {
  const char* __process = __ycxx_allocation_functions.__abi;
  const char* __mine = __ycxx::__detail::__own_allocation_functions.__abi;
  if (__process == __mine)
    return;
  const char* __p = __process;
  const char* __m = __mine;
  while (*__p != 0 && *__p == *__m)
    ++__p, ++__m;
  if (*__p == *__m)
    return;
  char __msg[256];
  char* __out = __msg;
  const char* __end = __msg + sizeof __msg - 1;
  __append(__out, __end, "libycxx: ABI version mismatch: this image was built against libycxx ");
  __append(__out, __end, __mine);
  __append(__out, __end, ", the process's first libycxx image against libycxx ");
  __append(__out, __end, __process);
  __append(__out, __end, "; every image of a process must use the same libycxx version");
  *__out = 0;
  if constexpr (__ycxx::__detail::__cfg::__layer::abort)
    ycxx_pal_abort(__msg);
  else
    __builtin_trap();
}
} // namespace
