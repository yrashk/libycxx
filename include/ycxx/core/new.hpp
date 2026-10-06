// libycxx core: <new>
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

struct destroying_delete_t {
  explicit destroying_delete_t() = default;
};
inline constexpr destroying_delete_t destroying_delete{};

enum class align_val_t : size_t {};

struct nothrow_t {
  explicit nothrow_t() = default;
};
// [new.syn]: declared extern. Defined by the ABI runtime (hosted) or libycxx-freestanding.a.
extern const nothrow_t nothrow;

using new_handler = void (*)();
new_handler get_new_handler() noexcept;
new_handler set_new_handler(new_handler __new_p) noexcept;

template <class _Tp>
[[nodiscard]] constexpr _Tp* launder(_Tp* p) noexcept {
  static_assert(!__is_function(_Tp) && !__ycxx::__detail::is_void_v<_Tp>, "std::launder of function or void pointer");
  return __builtin_launder(p);
}

inline constexpr size_t hardware_destructive_interference_size = 64;
inline constexpr size_t hardware_constructive_interference_size = 64;

} // namespace std

// Replaceable allocation functions. Defaults: the hosted runtime (via the PAL) or
// libycxx-freestanding.a (no heap); one function per archive member, so a program may replace
// any subset (src/runtime/new). libycxx's defaults are hidden and shared among the images that link
// libycxx through the allocation table (DECISIONS §2); a program's replacement is exported, as
// its other functions are: the compilers' implicit declarations give the forms they predeclare
// default visibility, and the nothrow forms say so explicitly, since a function otherwise takes
// the (hidden) visibility of its parameter type std::nothrow_t.
// Externally visible for GCC: under -fwhole-program it would otherwise localize a program's
// replacement, and the library's default functions (e.g. the sized delete, which calls the
// unsized one) would not reach it (GCC bugzilla 50594); Clang does not know the attribute, hence
// the -Wattributes push.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
[[nodiscard, __gnu__::__externally_visible__]] void* operator new(std::size_t size);
[[nodiscard, __gnu__::__externally_visible__]] void* operator new(std::size_t size, std::align_val_t alignment);
[[__gnu__::__visibility__("default")]] [[nodiscard, __gnu__::__externally_visible__]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
[[__gnu__::__visibility__("default")]] [[nodiscard, __gnu__::__externally_visible__]] void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[__gnu__::__externally_visible__]] void operator delete(void* ptr) noexcept;
[[__gnu__::__externally_visible__]] void operator delete(void* ptr, std::size_t size) noexcept;
[[__gnu__::__externally_visible__]] void operator delete(void* ptr, std::align_val_t alignment) noexcept;
[[__gnu__::__externally_visible__]] void operator delete(void* ptr, std::size_t size, std::align_val_t alignment) noexcept;
[[__gnu__::__visibility__("default")]] [[__gnu__::__externally_visible__]] void operator delete(void* ptr, const std::nothrow_t&) noexcept;
[[__gnu__::__visibility__("default")]] [[__gnu__::__externally_visible__]] void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[nodiscard, __gnu__::__externally_visible__]] void* operator new[](std::size_t size);
[[nodiscard, __gnu__::__externally_visible__]] void* operator new[](std::size_t size, std::align_val_t alignment);
[[__gnu__::__visibility__("default")]] [[nodiscard, __gnu__::__externally_visible__]] void* operator new[](std::size_t size, const std::nothrow_t&) noexcept;
[[__gnu__::__visibility__("default")]] [[nodiscard, __gnu__::__externally_visible__]] void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[__gnu__::__externally_visible__]] void operator delete[](void* ptr) noexcept;
[[__gnu__::__externally_visible__]] void operator delete[](void* ptr, std::size_t size) noexcept;
[[__gnu__::__externally_visible__]] void operator delete[](void* ptr, std::align_val_t alignment) noexcept;
[[__gnu__::__externally_visible__]] void operator delete[](void* ptr, std::size_t size, std::align_val_t alignment) noexcept;
[[__gnu__::__visibility__("default")]] [[__gnu__::__externally_visible__]] void operator delete[](void* ptr, const std::nothrow_t&) noexcept;
[[__gnu__::__visibility__("default")]] [[__gnu__::__externally_visible__]] void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;
#pragma GCC diagnostic pop


// Non-allocating forms (constexpr since C++26). Not replaceable and defined here, so hidden like
// the library's namespaces (DECISIONS §2).
[[nodiscard, __gnu__::__visibility__("hidden")]] constexpr void* operator new(std::size_t, void* ptr) noexcept { return ptr; }
[[nodiscard, __gnu__::__visibility__("hidden")]] constexpr void* operator new[](std::size_t, void* ptr) noexcept { return ptr; }
[[__gnu__::__visibility__("hidden")]] constexpr void operator delete(void*, void*) noexcept {}
[[__gnu__::__visibility__("hidden")]] constexpr void operator delete[](void*, void*) noexcept {}
