// libycxx core: <new>
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/error.hpp>

namespace [[gnu::visibility("hidden")]] std {

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
new_handler set_new_handler(new_handler new_p) noexcept;

template <class T>
[[nodiscard]] constexpr T* launder(T* p) noexcept {
  static_assert(!__is_function(T) && !ycxx::detail::is_void_v<T>, "std::launder of function or void pointer");
  return __builtin_launder(p);
}

inline constexpr size_t hardware_destructive_interference_size = 64;
inline constexpr size_t hardware_constructive_interference_size = 64;

} // namespace std

// Replaceable allocation functions. Defaults: the hosted runtime (via the PAL) or
// libycxx-freestanding.a (no heap); one function per archive member, so a program may replace
// any subset (src/runtime/new). Externally visible for GCC: under -fwhole-program it would
// otherwise localize a program's replacement, and the library's default functions (e.g. the
// sized delete, which calls the unsized one) would not reach it (GCC bugzilla 50594); Clang does not
// know the attribute, hence the -Wattributes push.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
[[nodiscard, gnu::externally_visible]] void* operator new(std::size_t size);
[[nodiscard, gnu::externally_visible]] void* operator new(std::size_t size, std::align_val_t alignment);
[[nodiscard, gnu::externally_visible]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
[[nodiscard, gnu::externally_visible]] void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[gnu::externally_visible]] void operator delete(void* ptr) noexcept;
[[gnu::externally_visible]] void operator delete(void* ptr, std::size_t size) noexcept;
[[gnu::externally_visible]] void operator delete(void* ptr, std::align_val_t alignment) noexcept;
[[gnu::externally_visible]] void operator delete(void* ptr, std::size_t size, std::align_val_t alignment) noexcept;
[[gnu::externally_visible]] void operator delete(void* ptr, const std::nothrow_t&) noexcept;
[[gnu::externally_visible]] void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[nodiscard, gnu::externally_visible]] void* operator new[](std::size_t size);
[[nodiscard, gnu::externally_visible]] void* operator new[](std::size_t size, std::align_val_t alignment);
[[nodiscard, gnu::externally_visible]] void* operator new[](std::size_t size, const std::nothrow_t&) noexcept;
[[nodiscard, gnu::externally_visible]] void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[gnu::externally_visible]] void operator delete[](void* ptr) noexcept;
[[gnu::externally_visible]] void operator delete[](void* ptr, std::size_t size) noexcept;
[[gnu::externally_visible]] void operator delete[](void* ptr, std::align_val_t alignment) noexcept;
[[gnu::externally_visible]] void operator delete[](void* ptr, std::size_t size, std::align_val_t alignment) noexcept;
[[gnu::externally_visible]] void operator delete[](void* ptr, const std::nothrow_t&) noexcept;
[[gnu::externally_visible]] void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;
#pragma GCC diagnostic pop


// Non-allocating forms (constexpr since C++26). Not replaceable and defined here, so hidden like
// the library's namespaces (DECISIONS §2).
[[nodiscard, gnu::visibility("hidden")]] constexpr void* operator new(std::size_t, void* ptr) noexcept { return ptr; }
[[nodiscard, gnu::visibility("hidden")]] constexpr void* operator new[](std::size_t, void* ptr) noexcept { return ptr; }
[[gnu::visibility("hidden")]] constexpr void operator delete(void*, void*) noexcept {}
[[gnu::visibility("hidden")]] constexpr void operator delete[](void*, void*) noexcept {}
