// libycxx core: generator ([coro.generator]).
//
// Nesting. Instead of the exposition-only stack of coroutine handles, the promises of a chain of
// recursively yielded generators link to each other: each promise knows its parent (the
// generator that yielded elements_of it) and the root (the generator being iterated), and the
// root knows the innermost active promise (top_). The iterator reads the value through
// root.top_ and resumes top_'s coroutine. Entering a nested generator and returning from it are
// symmetric transfers, so the depth of nesting costs no stack. A nested generator is owned by the
// awaitable in its parent's frame (so destroying the root destroys the chain); an exception
// leaving it is stored in its promise and rethrown in the parent by that awaitable.
//
// Frame allocation ([coro.generator.promise]/17-22): the frame is allocated as an array of
// gen_frame_unit (size and alignment __STDCPP_DEFAULT_NEW_ALIGNMENT__) with the allocator rebound
// to it. A copy of the allocator is stored after the frame unless it is always equal and default
// constructible. With Allocator = void the allocator type is erased: a deallocation function
// pointer is stored after the frame as well.
#pragma once

#include <ycxx/core/coroutine.hpp>
#include <ycxx/core/exception_ptr.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/ranges_to.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Ref, class _Val = void, class _Allocator = void>
class generator;
}}

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The part of a generator's promise that depends only on its yielded type, so that generators
// with different value or allocator types but the same yielded type can be nested.
template <class _Yielded>
struct __gen_promise_base {
  std::add_pointer_t<_Yielded> __value_ = nullptr;
  std::exception_ptr __except_;
  std::coroutine_handle<> __self_;
  __gen_promise_base* __parent_ = nullptr; // null for the root
  __gen_promise_base* __root_ = this;
  __gen_promise_base* __top_ = this; // meaningful in the root: the innermost active generator

  struct __final_awaiter {
    static constexpr bool await_ready() noexcept { return false; }
    template <class _Pp>
    static std::coroutine_handle<> await_suspend(std::coroutine_handle<_Pp> h) noexcept {
      __gen_promise_base& p = h.promise();
      if (p.__parent_ == nullptr)
        return std::noop_coroutine();
      p.__root_->__top_ = p.__parent_;
      return p.__parent_->__self_;
    }
    static constexpr void await_resume() noexcept {}
  };

  // co_yield of an lvalue when yielded is an rvalue reference: a copy kept in the frame.
  template <class _Vp>
  struct __copy_awaiter {
    _Vp __v;
    __gen_promise_base* p;
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept { p->__value_ = __builtin_addressof(__v); }
    static constexpr void await_resume() noexcept {}
  };

  // co_yield elements_of(generator): owns the nested generator.
  template <class _Gen>
  struct __nested_awaiter {
    _Gen __g;
    static constexpr bool await_ready() noexcept { return false; }
    template <class _Pp>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<_Pp> h) noexcept {
      __gen_promise_base& __parent = h.promise();
      auto handle = __g.__coroutine_;
      __gen_promise_base& __child = handle.promise();
      __child.__parent_ = __builtin_addressof(__parent);
      __child.__root_ = __parent.__root_;
      __parent.__root_->__top_ = __builtin_addressof(__child);
      return handle;
    }
    void await_resume() {
      __gen_promise_base& __child = __g.__coroutine_.promise();
      if (__child.__except_)
        std::rethrow_exception(static_cast<std::exception_ptr&&>(__child.__except_));
    }
  };
};

// The unit of frame allocation.
struct alignas(__cfg::__default_new_alignment) __gen_frame_unit {
  unsigned char bytes[__cfg::__default_new_alignment];
};
static_assert(sizeof(__gen_frame_unit) == __cfg::__default_new_alignment);

using __gen_dealloc_fn = void (*)(void*, std::size_t) noexcept;

constexpr std::size_t __gen_round_up(std::size_t n, std::size_t a) noexcept { return (n + a - 1) / a * a; }

// Allocation of a frame of n bytes with B (an allocator of gen_frame_unit). Erased: the frame
// also records how to deallocate it (Allocator = void).
template <class _Bp, bool _Erased>
struct __gen_frame {
  using __traits = std::allocator_traits<_Bp>;
  static_assert(std::is_pointer_v<typename __traits::pointer>,
                "std::generator: the allocator's pointer type must be a pointer type");
  static_assert(alignof(_Bp) <= alignof(__gen_frame_unit), "std::generator: over-aligned allocator");
  static constexpr bool __stateless = __traits::is_always_equal::value && std::is_default_constructible_v<_Bp>;

  static constexpr std::size_t __fn_offset(std::size_t n) noexcept { return __gen_round_up(n, alignof(__gen_dealloc_fn)); }
  static constexpr std::size_t __alloc_offset(std::size_t n) noexcept {
    return __gen_round_up(_Erased ? __fn_offset(n) + sizeof(__gen_dealloc_fn) : n, alignof(_Bp));
  }
  static constexpr std::size_t __units(std::size_t n) noexcept {
    std::size_t __total = !__stateless ? __alloc_offset(n) + sizeof(_Bp) : _Erased ? __fn_offset(n) + sizeof(__gen_dealloc_fn) : n;
    return (__total + sizeof(__gen_frame_unit) - 1) / sizeof(__gen_frame_unit);
  }

  static void* allocate(_Bp b, std::size_t n) {
    __gen_frame_unit* p = __traits::allocate(b, __units(n));
    unsigned char* c = reinterpret_cast<unsigned char*>(p);
    if constexpr (_Erased)
      std::construct_at(reinterpret_cast<__gen_dealloc_fn*>(c + __fn_offset(n)), &deallocate);
    if constexpr (!__stateless)
      std::construct_at(reinterpret_cast<_Bp*>(c + __alloc_offset(n)), static_cast<_Bp&&>(b));
    return p;
  }
  static void deallocate(void* p, std::size_t n) noexcept {
    if constexpr (__stateless) {
      _Bp b;
      __traits::deallocate(b, static_cast<__gen_frame_unit*>(p), __units(n));
    } else {
      _Bp* __stored = reinterpret_cast<_Bp*>(static_cast<unsigned char*>(p) + __alloc_offset(n));
      _Bp b(static_cast<_Bp&&>(*__stored));
      std::destroy_at(__stored);
      __traits::deallocate(b, static_cast<__gen_frame_unit*>(p), __units(n));
    }
  }
};

// Deallocation of a frame allocated through gen_frame<B, true>, whatever B was.
inline void __gen_erased_deallocate(void* p, std::size_t n) noexcept {
  unsigned char* c = static_cast<unsigned char*>(p);
  __gen_dealloc_fn __f = *reinterpret_cast<__gen_dealloc_fn*>(c + __gen_round_up(n, alignof(__gen_dealloc_fn)));
  __f(p, n);
}

// [coro.generator.class]/1.1: Allocator is void or its pointer type is a pointer type.
template <class _Ap>
inline constexpr bool __gen_allocator_ok = std::is_pointer_v<typename std::allocator_traits<_Ap>::pointer>;
template <>
inline constexpr bool __gen_allocator_ok<void> = true;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Ref, class _Val, class _Allocator>
class generator : public ranges::view_interface<generator<_Ref, _Val, _Allocator>> {
  using value = conditional_t<is_void_v<_Val>, remove_cvref_t<_Ref>, _Val>;
  using reference = conditional_t<is_void_v<_Val>, _Ref&&, _Ref>;
  using __rref = conditional_t<is_reference_v<reference>, remove_reference_t<reference>&&, reference>;

  // [coro.generator.class]/1 Mandates.
  static_assert(__ycxx::__detail::__gen_allocator_ok<_Allocator>,
                "std::generator: allocator_traits<Allocator>::pointer must be a pointer type");
  static_assert(is_object_v<value> && is_same_v<value, remove_cv_t<value>>,
                "std::generator: the value type must be a cv-unqualified object type");
  static_assert(is_reference_v<reference> ||
                    (is_object_v<reference> && is_same_v<reference, remove_cv_t<reference>> &&
                     copy_constructible<reference>),
                "std::generator: the reference type must be a reference or a copy-constructible "
                "cv-unqualified object type");
  static_assert(common_reference_with<reference&&, value&> && common_reference_with<reference&&, __rref&&> &&
                    common_reference_with<__rref&&, const value&>,
                "std::generator: the reference and value types need a common reference");

  class iterator;

public:
  using yielded = conditional_t<is_reference_v<reference>, reference, const reference&>;
  class promise_type;

private:
  template <class _Yp>
  friend struct __ycxx::__detail::__gen_promise_base;

  coroutine_handle<promise_type> __coroutine_ = nullptr;

  explicit generator(coroutine_handle<promise_type> h) noexcept : __coroutine_(h) {}

public:
  generator(const generator&) = delete;
  generator(generator&& other) noexcept : __coroutine_(other.__coroutine_) { other.__coroutine_ = nullptr; }
  ~generator() {
    if (__coroutine_)
      __coroutine_.destroy();
  }
  generator& operator=(generator other) noexcept {
    coroutine_handle<promise_type> t = __coroutine_;
    __coroutine_ = other.__coroutine_;
    other.__coroutine_ = t;
    return *this;
  }

  iterator begin() {
    __ycxx::__detail::__precondition(__coroutine_ && !__coroutine_.done(),
                               "std::generator::begin: no coroutine suspended at its initial suspend point");
    __coroutine_.resume();
    return iterator(__coroutine_);
  }
  default_sentinel_t end() const noexcept { return default_sentinel; }
};

template <class _Ref, class _Val, class _Allocator>
class generator<_Ref, _Val, _Allocator>::promise_type : public __ycxx::__detail::__gen_promise_base<yielded> {
  using base = __ycxx::__detail::__gen_promise_base<yielded>;
  template <class _Bp, bool _Erased>
  using __frame = __ycxx::__detail::__gen_frame<_Bp, _Erased>;
  template <class _Ap>
  using __unit_alloc = typename allocator_traits<_Ap>::template rebind_alloc<__ycxx::__detail::__gen_frame_unit>;
  static constexpr bool __erased = is_void_v<_Allocator>;

  template <class _Ap>
  static void* allocate(const _Ap& a, size_t size) {
    return __frame<__unit_alloc<_Ap>, __erased>::allocate(__unit_alloc<_Ap>(a), size);
  }
  template <class _Alloc>
  static void* __allocate_with(const _Alloc& __alloc, size_t size) {
    static_assert(is_void_v<_Allocator> || convertible_to<const _Alloc&, _Allocator>,
                  "std::generator: the allocator argument must convert to Allocator");
    using _Ap = conditional_t<is_void_v<_Allocator>, _Alloc, _Allocator>;
    return allocate(_Ap(__alloc), size);
  }

public:
  generator get_return_object() noexcept {
    auto h = coroutine_handle<promise_type>::from_promise(*this);
    this->__self_ = h;
    return generator(h);
  }
  suspend_always initial_suspend() const noexcept { return {}; }
  auto final_suspend() noexcept { return typename base::__final_awaiter{}; }

  suspend_always yield_value(yielded __val) noexcept {
    this->__value_ = __builtin_addressof(__val);
    return {};
  }
  auto yield_value(const remove_reference_t<yielded>& __lval)
    requires is_rvalue_reference_v<yielded> &&
             constructible_from<remove_cvref_t<yielded>, const remove_reference_t<yielded>&>
  {
    return typename base::template __copy_awaiter<remove_cvref_t<yielded>>{remove_cvref_t<yielded>(__lval), this};
  }
  template <class _R2, class _V2, class _Alloc2, class _Unused>
    requires same_as<typename generator<_R2, _V2, _Alloc2>::yielded, yielded>
  auto yield_value(ranges::elements_of<generator<_R2, _V2, _Alloc2>&&, _Unused> __g) noexcept {
    return typename base::template __nested_awaiter<generator<_R2, _V2, _Alloc2>>{
        static_cast<generator<_R2, _V2, _Alloc2>&&>(__g.range)};
  }
  template <class _R2, class _V2, class _Alloc2, class _Unused>
    requires same_as<typename generator<_R2, _V2, _Alloc2>::yielded, yielded>
  auto yield_value(ranges::elements_of<generator<_R2, _V2, _Alloc2>&, _Unused> __g) noexcept {
    return typename base::template __nested_awaiter<generator<_R2, _V2, _Alloc2>>{
        static_cast<generator<_R2, _V2, _Alloc2>&&>(__g.range)};
  }
  template <ranges::input_range _Rp, class _Alloc>
    requires convertible_to<ranges::range_reference_t<_Rp>, yielded>
  auto yield_value(ranges::elements_of<_Rp, _Alloc> r) {
    auto __nested = [](allocator_arg_t, _Alloc, ranges::iterator_t<_Rp> i,
                     ranges::sentinel_t<_Rp> s) -> generator<yielded, void, _Alloc> {
      for (; i != s; ++i)
        co_yield static_cast<yielded>(*i);
    };
    return yield_value(
        ranges::elements_of(__nested(allocator_arg, r.allocator, ranges::begin(r.range), ranges::end(r.range))));
  }

  void await_transform() = delete;
  void return_void() const noexcept {}
  void unhandled_exception() {
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      if (this->__parent_ == nullptr)
        throw;
      this->__except_ = current_exception();
    }
  }

  void* operator new(size_t size)
    requires same_as<_Allocator, void> || default_initializable<_Allocator>
  {
    if constexpr (is_void_v<_Allocator>)
      return allocate(allocator<void>(), size);
    else
      return allocate(_Allocator(), size);
  }
  template <class _Alloc, class... _Args>
  void* operator new(size_t size, allocator_arg_t, const _Alloc& __alloc, const _Args&...) {
    return __allocate_with(__alloc, size);
  }
  template <class _This, class _Alloc, class... _Args>
  void* operator new(size_t size, const _This&, allocator_arg_t, const _Alloc& __alloc, const _Args&...) {
    return __allocate_with(__alloc, size);
  }
  void operator delete(void* pointer, size_t size) noexcept {
    if constexpr (is_void_v<_Allocator>)
      __ycxx::__detail::__gen_erased_deallocate(pointer, size);
    else
      __frame<__unit_alloc<_Allocator>, false>::deallocate(pointer, size);
  }
};

template <class _Ref, class _Val, class _Allocator>
class generator<_Ref, _Val, _Allocator>::iterator {
  friend class generator;
  coroutine_handle<promise_type> __coroutine_;

  explicit iterator(coroutine_handle<promise_type> h) noexcept : __coroutine_(h) {}

public:
  using value_type = value;
  using difference_type = ptrdiff_t;

  iterator(iterator&& other) noexcept : __coroutine_(other.__coroutine_) { other.__coroutine_ = nullptr; }
  iterator& operator=(iterator&& other) noexcept {
    __coroutine_ = other.__coroutine_;
    other.__coroutine_ = nullptr;
    return *this;
  }
  reference operator*() const noexcept(is_nothrow_copy_constructible_v<reference>) {
    __ycxx::__detail::__precondition(!__coroutine_.done(), "std::generator::iterator::operator*: at the end");
    return static_cast<reference>(*__coroutine_.promise().__top_->__value_);
  }
  iterator& operator++() {
    __ycxx::__detail::__precondition(!__coroutine_.done(), "std::generator::iterator::operator++: at the end");
    __coroutine_.promise().__top_->__self_.resume();
    return *this;
  }
  void operator++(int) { ++*this; }
  friend bool operator==(const iterator& i, default_sentinel_t) { return i.__coroutine_.done(); }
};

namespace pmr {
template <class _Ref, class _Val = void>
using generator = std::generator<_Ref, _Val, polymorphic_allocator<>>;
}

}} // namespace std
