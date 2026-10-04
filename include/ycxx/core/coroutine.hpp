// libycxx core: <coroutine>
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/compare.hpp>

namespace std {

// [coroutine.traits]
template <class R, class... Args>
struct coroutine_traits {};
template <class R, class... Args>
  requires requires { typename R::promise_type; }
struct coroutine_traits<R, Args...> {
  using promise_type = typename R::promise_type;
};

template <class Promise = void>
struct coroutine_handle;

template <>
struct coroutine_handle<void> {
  constexpr coroutine_handle() noexcept = default;
  constexpr coroutine_handle(nullptr_t) noexcept {}
  coroutine_handle& operator=(nullptr_t) noexcept {
    ptr_ = nullptr;
    return *this;
  }

  constexpr void* address() const noexcept { return ptr_; }
  static constexpr coroutine_handle from_address(void* addr) noexcept {
    coroutine_handle h;
    h.ptr_ = addr;
    return h;
  }

  constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool done() const { return __builtin_coro_done(ptr_); }

  void operator()() const { resume(); }
  void resume() const { __builtin_coro_resume(ptr_); }
  void destroy() const { __builtin_coro_destroy(ptr_); }

private:
  void* ptr_ = nullptr;
};

template <class Promise>
struct coroutine_handle {
  constexpr coroutine_handle() noexcept = default;
  constexpr coroutine_handle(nullptr_t) noexcept {}
  static coroutine_handle from_promise(Promise& p) {
    coroutine_handle h;
    // Promise may be cv-qualified ([coroutine.handle.con]/2); the builtin takes a void*.
    h.ptr_ = __builtin_coro_promise(const_cast<void*>(static_cast<const volatile void*>(__builtin_addressof(p))),
                                    alignof(Promise), true);
    return h;
  }
  coroutine_handle& operator=(nullptr_t) noexcept {
    ptr_ = nullptr;
    return *this;
  }

  constexpr void* address() const noexcept { return ptr_; }
  static constexpr coroutine_handle from_address(void* addr) noexcept {
    coroutine_handle h;
    h.ptr_ = addr;
    return h;
  }

  constexpr operator coroutine_handle<>() const noexcept { return coroutine_handle<>::from_address(ptr_); }

  constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool done() const { return __builtin_coro_done(ptr_); }

  void operator()() const { resume(); }
  void resume() const { __builtin_coro_resume(ptr_); }
  void destroy() const { __builtin_coro_destroy(ptr_); }

  Promise& promise() const {
    return *static_cast<Promise*>(__builtin_coro_promise(ptr_, alignof(Promise), false));
  }

private:
  void* ptr_ = nullptr;
};

constexpr bool operator==(coroutine_handle<> x, coroutine_handle<> y) noexcept { return x.address() == y.address(); }
constexpr strong_ordering operator<=>(coroutine_handle<> x, coroutine_handle<> y) noexcept {
  return compare_three_way()(x.address(), y.address());
}

// [coroutine.noop]
struct noop_coroutine_promise {};

template <>
struct coroutine_handle<noop_coroutine_promise> {
  constexpr operator coroutine_handle<>() const noexcept { return coroutine_handle<>::from_address(ptr_); }
  constexpr explicit operator bool() const noexcept { return true; }
  constexpr bool done() const noexcept { return false; }
  constexpr void operator()() const noexcept {}
  constexpr void resume() const noexcept {}
  constexpr void destroy() const noexcept {}
  noop_coroutine_promise& promise() const noexcept {
    return *static_cast<noop_coroutine_promise*>(__builtin_coro_promise(ptr_, alignof(noop_coroutine_promise), false));
  }
  constexpr void* address() const noexcept { return ptr_; }

private:
  friend coroutine_handle noop_coroutine() noexcept;
  explicit coroutine_handle(void* p) noexcept : ptr_(p) {}
  void* ptr_;
};
using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

} // namespace std

namespace ycxx::detail {
// A coroutine frame whose resume/destroy entries do nothing, laid out like the frames both
// compilers create: {resume fn, destroy fn, promise}.
struct noop_frame {
  using fn = void (*)(void*);
  fn resume;
  fn destroy;
  std::noop_coroutine_promise promise;
  static void nop(void*) noexcept {}
};
inline noop_frame noop_frame_instance{&noop_frame::nop, &noop_frame::nop, {}};
} // namespace ycxx::detail

namespace std {
inline noop_coroutine_handle noop_coroutine() noexcept {
  return noop_coroutine_handle(&ycxx::detail::noop_frame_instance);
}

// [coroutine.trivial.awaitables]
struct suspend_never {
  constexpr bool await_ready() const noexcept { return true; }
  constexpr void await_suspend(coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};
struct suspend_always {
  constexpr bool await_ready() const noexcept { return false; }
  constexpr void await_suspend(coroutine_handle<>) const noexcept {}
  constexpr void await_resume() const noexcept {}
};

template <class T>
struct hash;
template <class P>
struct hash<coroutine_handle<P>> {
  size_t operator()(const coroutine_handle<P>& h) const noexcept {
    return static_cast<size_t>(reinterpret_cast<__UINTPTR_TYPE__>(h.address()));
  }
};

} // namespace std
