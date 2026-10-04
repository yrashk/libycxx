// libycxx core: <new>
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/error.hpp>

namespace std {

struct destroying_delete_t {
  explicit destroying_delete_t() = default;
};
inline constexpr destroying_delete_t destroying_delete{};

enum class align_val_t : size_t {};

struct nothrow_t {
  explicit nothrow_t() = default;
};
// [new.syn]: declared extern. Defined by libsupc++ (hosted) or libycxx-freestanding.a.
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
// any subset (src/runtime/new).
[[nodiscard]] void* operator new(std::size_t size);
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, std::size_t size) noexcept;
void operator delete(void* ptr, std::align_val_t alignment) noexcept;
void operator delete(void* ptr, std::size_t size, std::align_val_t alignment) noexcept;
void operator delete(void* ptr, const std::nothrow_t&) noexcept;
void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new[](std::size_t size);
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new[](std::size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, std::size_t size) noexcept;
void operator delete[](void* ptr, std::align_val_t alignment) noexcept;
void operator delete[](void* ptr, std::size_t size, std::align_val_t alignment) noexcept;
void operator delete[](void* ptr, const std::nothrow_t&) noexcept;
void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;


// Non-allocating forms (constexpr since C++26).
[[nodiscard]] constexpr void* operator new(std::size_t, void* ptr) noexcept { return ptr; }
[[nodiscard]] constexpr void* operator new[](std::size_t, void* ptr) noexcept { return ptr; }
constexpr void operator delete(void*, void*) noexcept {}
constexpr void operator delete[](void*, void*) noexcept {}
