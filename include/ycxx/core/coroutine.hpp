// libycxx core: <coroutine>
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/compare.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [coroutine.traits]
template <class _Rp, class... _Args>
struct coroutine_traits {};
template <class _Rp, class... _Args>
  requires requires { typename _Rp::promise_type; }
struct coroutine_traits<_Rp, _Args...> {
  using promise_type = typename _Rp::promise_type;
};

template <class _Promise = void>
struct coroutine_handle;

template <>
struct coroutine_handle<void> {
  constexpr coroutine_handle() noexcept = default;
  constexpr coroutine_handle(nullptr_t) noexcept {}
  coroutine_handle& operator=(nullptr_t) noexcept {
    __ptr_ = nullptr;
    return *this;
  }

  constexpr void* address() const noexcept { return __ptr_; }
  static constexpr coroutine_handle from_address(void* __addr) noexcept {
    coroutine_handle h;
    h.__ptr_ = __addr;
    return h;
  }

  constexpr explicit operator bool() const noexcept { return __ptr_ != nullptr; }
  bool done() const { return __builtin_coro_done(__ptr_); }

  void operator()() const { resume(); }
  void resume() const { __builtin_coro_resume(__ptr_); }
  void destroy() const { __builtin_coro_destroy(__ptr_); }

private:
  void* __ptr_ = nullptr;
};

template <class _Promise>
struct coroutine_handle {
  constexpr coroutine_handle() noexcept = default;
  constexpr coroutine_handle(nullptr_t) noexcept {}
  static coroutine_handle from_promise(_Promise& p) {
    coroutine_handle h;
    // Promise may be cv-qualified ([coroutine.handle.con]/2); the builtin takes a void*.
    h.__ptr_ = __builtin_coro_promise(const_cast<void*>(static_cast<const volatile void*>(__builtin_addressof(p))),
                                    alignof(_Promise), true);
    return h;
  }
  coroutine_handle& operator=(nullptr_t) noexcept {
    __ptr_ = nullptr;
    return *this;
  }

  constexpr void* address() const noexcept { return __ptr_; }
  static constexpr coroutine_handle from_address(void* __addr) noexcept {
    coroutine_handle h;
    h.__ptr_ = __addr;
    return h;
  }

  constexpr operator coroutine_handle<>() const noexcept { return coroutine_handle<>::from_address(__ptr_); }

  constexpr explicit operator bool() const noexcept { return __ptr_ != nullptr; }
  bool done() const { return __builtin_coro_done(__ptr_); }

  void operator()() const { resume(); }
  void resume() const { __builtin_coro_resume(__ptr_); }
  void destroy() const { __builtin_coro_destroy(__ptr_); }

  _Promise& promise() const {
    return *static_cast<_Promise*>(__builtin_coro_promise(__ptr_, alignof(_Promise), false));
  }

private:
  void* __ptr_ = nullptr;
};

constexpr bool operator==(coroutine_handle<> __x, coroutine_handle<> y) noexcept { return __x.address() == y.address(); }
constexpr strong_ordering operator<=>(coroutine_handle<> __x, coroutine_handle<> y) noexcept {
  return compare_three_way()(__x.address(), y.address());
}

// [coroutine.noop]
struct noop_coroutine_promise {};

template <>
struct coroutine_handle<noop_coroutine_promise> {
  constexpr operator coroutine_handle<>() const noexcept { return coroutine_handle<>::from_address(__ptr_); }
  constexpr explicit operator bool() const noexcept { return true; }
  constexpr bool done() const noexcept { return false; }
  constexpr void operator()() const noexcept {}
  constexpr void resume() const noexcept {}
  constexpr void destroy() const noexcept {}
  noop_coroutine_promise& promise() const noexcept {
    return *static_cast<noop_coroutine_promise*>(__builtin_coro_promise(__ptr_, alignof(noop_coroutine_promise), false));
  }
  constexpr void* address() const noexcept { return __ptr_; }

private:
  friend coroutine_handle noop_coroutine() noexcept;
  explicit coroutine_handle(void* p) noexcept : __ptr_(p) {}
  void* __ptr_;
};
using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// A coroutine frame whose resume/destroy entries do nothing, laid out like the frames both
// compilers create: {resume fn, destroy fn, promise}.
struct __noop_frame {
  using __fn = void (*)(void*);
  __fn resume;
  __fn destroy;
  std::noop_coroutine_promise promise;
  static void __nop(void*) noexcept {}
};
inline __noop_frame __noop_frame_instance{&__noop_frame::__nop, &__noop_frame::__nop, {}};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
inline noop_coroutine_handle noop_coroutine() noexcept {
  return noop_coroutine_handle(&__ycxx::__detail::__noop_frame_instance);
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

template <class _Tp>
struct hash;
template <class _Pp>
struct hash<coroutine_handle<_Pp>> {
  size_t operator()(const coroutine_handle<_Pp>& h) const noexcept {
    return static_cast<size_t>(reinterpret_cast<__UINTPTR_TYPE__>(h.address()));
  }
};

} // namespace std
