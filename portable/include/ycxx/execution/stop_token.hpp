// The in-place stop-token family for projects using the toolchain's standard library.
#pragma once

#include <atomic>
#include <concepts>
#include <cstdint>
#include <optional>
#include <thread>
#include <type_traits>

namespace __ycxx_portable_stop_token {

struct __stop_callback_node {
  __stop_callback_node* next = nullptr;
  __stop_callback_node** prev = nullptr;
  void (*invoke)(__stop_callback_node*) noexcept = nullptr;
  bool* __removed = nullptr;
  std::atomic_flag done{};
};

struct __stop_platform {
  using __word = std::uint32_t;
  // The empty identity keeps the source's default constructor usable in constant evaluation.
  using __thread_id = std::optional<std::thread::id>;

  static __thread_id __self() noexcept { return std::this_thread::get_id(); }
  static void __yield() noexcept { std::this_thread::yield(); }

  static bool __finished(const std::atomic_flag& __done) noexcept {
    return __done.test(std::memory_order_acquire);
  }
  static void __finish(std::atomic_flag& __done) noexcept {
    // The destructor can free the callback after this operation. This is our last access to it.
    (void)__done.test_and_set(std::memory_order_release);
  }
  static void __wait(const std::atomic_flag& __done) noexcept {
    while (!__finished(__done))
      __yield();
  }
};

#include <ycxx/core/stop_state.inc>

} // namespace __ycxx_portable_stop_token

namespace ycxx::execution {

class inplace_stop_source;
template <class _CallbackFn>
class inplace_stop_callback;

using __inplace_stop_state = ::__ycxx_portable_stop_token::__stop_state;
using __inplace_stop_callback_node = ::__ycxx_portable_stop_token::__stop_callback_node;

#include <ycxx/core/inplace_stop_token.inc>

} // namespace ycxx::execution
