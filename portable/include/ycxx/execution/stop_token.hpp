// The in-place stop-token family for projects using the toolchain's standard library.
#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>

namespace __ycxx_portable_stop_token {

struct __stop_callback_node {
  __stop_callback_node* next = nullptr;
  __stop_callback_node** prev = nullptr;
  void (*invoke)(__stop_callback_node*) noexcept = nullptr;
  bool* __removed = nullptr;
  alignas(std::atomic_ref<unsigned char>::required_alignment) unsigned char done = 0;
};

// A waiter may free the callback after seeing its completion flag. Keep the notifier
// and the last part of the wait serialized by a lock that outlives the callback.
class __completion_lock {
  std::atomic_flag __locked_;

public:
  void lock() noexcept {
    while (__locked_.test_and_set(std::memory_order_acquire))
      __locked_.wait(true, std::memory_order_relaxed);
  }
  void unlock() noexcept {
    __locked_.clear(std::memory_order_release);
    __locked_.notify_one();
  }
};

struct __stop_platform {
  using __word = std::uint32_t;
  // The empty identity keeps the source's default constructor usable in constant evaluation.
  using __thread_id = std::optional<std::thread::id>;

  static __thread_id __self() noexcept { return std::this_thread::get_id(); }
  static void __yield() noexcept { std::this_thread::yield(); }

  static bool __finished(unsigned char& __done) noexcept {
    return std::atomic_ref(__done).load(std::memory_order_acquire) != 0;
  }
  static void __finish(unsigned char& __done) noexcept {
    const std::lock_guard __lock(__completion(__done));
    const std::atomic_ref __flag(__done);
    __flag.store(1, std::memory_order_release);
    __flag.notify_all();
  }
  static void __wait(unsigned char& __done) noexcept {
    auto& __gate = __completion(__done);
    std::atomic_ref(__done).wait(0, std::memory_order_acquire);
    const std::lock_guard __lock(__gate);
  }

private:
  static __completion_lock& __completion(const unsigned char& __done) noexcept {
    static constexpr std::size_t __count = 64;
    static constinit __completion_lock __locks[__count]{};
    const auto __address = reinterpret_cast<std::uintptr_t>(&__done);
    return __locks[(__address / alignof(void*)) % __count];
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
