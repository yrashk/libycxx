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

namespace std {
template <class Ref, class Val = void, class Allocator = void>
class generator;
}

namespace ycxx::detail {

// The part of a generator's promise that depends only on its yielded type, so that generators
// with different value or allocator types but the same yielded type can be nested.
template <class Yielded>
struct gen_promise_base {
  std::add_pointer_t<Yielded> value_ = nullptr;
  std::exception_ptr except_;
  std::coroutine_handle<> self_;
  gen_promise_base* parent_ = nullptr; // null for the root
  gen_promise_base* root_ = this;
  gen_promise_base* top_ = this; // meaningful in the root: the innermost active generator

  struct final_awaiter {
    static constexpr bool await_ready() noexcept { return false; }
    template <class P>
    static std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h) noexcept {
      gen_promise_base& p = h.promise();
      if (p.parent_ == nullptr)
        return std::noop_coroutine();
      p.root_->top_ = p.parent_;
      return p.parent_->self_;
    }
    static constexpr void await_resume() noexcept {}
  };

  // co_yield of an lvalue when yielded is an rvalue reference: a copy kept in the frame.
  template <class V>
  struct copy_awaiter {
    V v;
    gen_promise_base* p;
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept { p->value_ = __builtin_addressof(v); }
    static constexpr void await_resume() noexcept {}
  };

  // co_yield elements_of(generator): owns the nested generator.
  template <class Gen>
  struct nested_awaiter {
    Gen g;
    static constexpr bool await_ready() noexcept { return false; }
    template <class P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h) noexcept {
      gen_promise_base& parent = h.promise();
      auto handle = g.coroutine_;
      gen_promise_base& child = handle.promise();
      child.parent_ = __builtin_addressof(parent);
      child.root_ = parent.root_;
      parent.root_->top_ = __builtin_addressof(child);
      return handle;
    }
    void await_resume() {
      gen_promise_base& child = g.coroutine_.promise();
      if (child.except_)
        std::rethrow_exception(static_cast<std::exception_ptr&&>(child.except_));
    }
  };
};

// The unit of frame allocation.
struct alignas(cfg::default_new_alignment) gen_frame_unit {
  unsigned char bytes[cfg::default_new_alignment];
};
static_assert(sizeof(gen_frame_unit) == cfg::default_new_alignment);

using gen_dealloc_fn = void (*)(void*, std::size_t) noexcept;

constexpr std::size_t gen_round_up(std::size_t n, std::size_t a) noexcept { return (n + a - 1) / a * a; }

// Allocation of a frame of n bytes with B (an allocator of gen_frame_unit). Erased: the frame
// also records how to deallocate it (Allocator = void).
template <class B, bool Erased>
struct gen_frame {
  using traits = std::allocator_traits<B>;
  static_assert(std::is_pointer_v<typename traits::pointer>,
                "std::generator: the allocator's pointer type must be a pointer type");
  static_assert(alignof(B) <= alignof(gen_frame_unit), "std::generator: over-aligned allocator");
  static constexpr bool stateless = traits::is_always_equal::value && std::is_default_constructible_v<B>;

  static constexpr std::size_t fn_offset(std::size_t n) noexcept { return gen_round_up(n, alignof(gen_dealloc_fn)); }
  static constexpr std::size_t alloc_offset(std::size_t n) noexcept {
    return gen_round_up(Erased ? fn_offset(n) + sizeof(gen_dealloc_fn) : n, alignof(B));
  }
  static constexpr std::size_t units(std::size_t n) noexcept {
    std::size_t total = !stateless ? alloc_offset(n) + sizeof(B) : Erased ? fn_offset(n) + sizeof(gen_dealloc_fn) : n;
    return (total + sizeof(gen_frame_unit) - 1) / sizeof(gen_frame_unit);
  }

  static void* allocate(B b, std::size_t n) {
    gen_frame_unit* p = traits::allocate(b, units(n));
    unsigned char* c = reinterpret_cast<unsigned char*>(p);
    if constexpr (Erased)
      std::construct_at(reinterpret_cast<gen_dealloc_fn*>(c + fn_offset(n)), &deallocate);
    if constexpr (!stateless)
      std::construct_at(reinterpret_cast<B*>(c + alloc_offset(n)), static_cast<B&&>(b));
    return p;
  }
  static void deallocate(void* p, std::size_t n) noexcept {
    if constexpr (stateless) {
      B b;
      traits::deallocate(b, static_cast<gen_frame_unit*>(p), units(n));
    } else {
      B* stored = reinterpret_cast<B*>(static_cast<unsigned char*>(p) + alloc_offset(n));
      B b(static_cast<B&&>(*stored));
      std::destroy_at(stored);
      traits::deallocate(b, static_cast<gen_frame_unit*>(p), units(n));
    }
  }
};

// Deallocation of a frame allocated through gen_frame<B, true>, whatever B was.
inline void gen_erased_deallocate(void* p, std::size_t n) noexcept {
  unsigned char* c = static_cast<unsigned char*>(p);
  gen_dealloc_fn f = *reinterpret_cast<gen_dealloc_fn*>(c + gen_round_up(n, alignof(gen_dealloc_fn)));
  f(p, n);
}

// [coro.generator.class]/1.1: Allocator is void or its pointer type is a pointer type.
template <class A>
inline constexpr bool gen_allocator_ok = std::is_pointer_v<typename std::allocator_traits<A>::pointer>;
template <>
inline constexpr bool gen_allocator_ok<void> = true;

} // namespace ycxx::detail

namespace std {

template <class Ref, class Val, class Allocator>
class generator : public ranges::view_interface<generator<Ref, Val, Allocator>> {
  using value = conditional_t<is_void_v<Val>, remove_cvref_t<Ref>, Val>;
  using reference = conditional_t<is_void_v<Val>, Ref&&, Ref>;
  using rref = conditional_t<is_reference_v<reference>, remove_reference_t<reference>&&, reference>;

  // [coro.generator.class]/1 Mandates.
  static_assert(ycxx::detail::gen_allocator_ok<Allocator>,
                "std::generator: allocator_traits<Allocator>::pointer must be a pointer type");
  static_assert(is_object_v<value> && is_same_v<value, remove_cv_t<value>>,
                "std::generator: the value type must be a cv-unqualified object type");
  static_assert(is_reference_v<reference> ||
                    (is_object_v<reference> && is_same_v<reference, remove_cv_t<reference>> &&
                     copy_constructible<reference>),
                "std::generator: the reference type must be a reference or a copy-constructible "
                "cv-unqualified object type");
  static_assert(common_reference_with<reference&&, value&> && common_reference_with<reference&&, rref&&> &&
                    common_reference_with<rref&&, const value&>,
                "std::generator: the reference and value types need a common reference");

  class iterator;

public:
  using yielded = conditional_t<is_reference_v<reference>, reference, const reference&>;
  class promise_type;

private:
  template <class Y>
  friend struct ycxx::detail::gen_promise_base;

  coroutine_handle<promise_type> coroutine_ = nullptr;

  explicit generator(coroutine_handle<promise_type> h) noexcept : coroutine_(h) {}

public:
  generator(const generator&) = delete;
  generator(generator&& other) noexcept : coroutine_(other.coroutine_) { other.coroutine_ = nullptr; }
  ~generator() {
    if (coroutine_)
      coroutine_.destroy();
  }
  generator& operator=(generator other) noexcept {
    coroutine_handle<promise_type> t = coroutine_;
    coroutine_ = other.coroutine_;
    other.coroutine_ = t;
    return *this;
  }

  iterator begin() {
    ycxx::detail::precondition(coroutine_ && !coroutine_.done(),
                               "std::generator::begin: no coroutine suspended at its initial suspend point");
    coroutine_.resume();
    return iterator(coroutine_);
  }
  default_sentinel_t end() const noexcept { return default_sentinel; }
};

template <class Ref, class Val, class Allocator>
class generator<Ref, Val, Allocator>::promise_type : public ycxx::detail::gen_promise_base<yielded> {
  using base = ycxx::detail::gen_promise_base<yielded>;
  template <class B, bool Erased>
  using frame = ycxx::detail::gen_frame<B, Erased>;
  template <class A>
  using unit_alloc = typename allocator_traits<A>::template rebind_alloc<ycxx::detail::gen_frame_unit>;
  static constexpr bool erased = is_void_v<Allocator>;

  template <class A>
  static void* allocate(const A& a, size_t size) {
    return frame<unit_alloc<A>, erased>::allocate(unit_alloc<A>(a), size);
  }
  template <class Alloc>
  static void* allocate_with(const Alloc& alloc, size_t size) {
    static_assert(is_void_v<Allocator> || convertible_to<const Alloc&, Allocator>,
                  "std::generator: the allocator argument must convert to Allocator");
    using A = conditional_t<is_void_v<Allocator>, Alloc, Allocator>;
    return allocate(A(alloc), size);
  }

public:
  generator get_return_object() noexcept {
    auto h = coroutine_handle<promise_type>::from_promise(*this);
    this->self_ = h;
    return generator(h);
  }
  suspend_always initial_suspend() const noexcept { return {}; }
  auto final_suspend() noexcept { return typename base::final_awaiter{}; }

  suspend_always yield_value(yielded val) noexcept {
    this->value_ = __builtin_addressof(val);
    return {};
  }
  auto yield_value(const remove_reference_t<yielded>& lval)
    requires is_rvalue_reference_v<yielded> &&
             constructible_from<remove_cvref_t<yielded>, const remove_reference_t<yielded>&>
  {
    return typename base::template copy_awaiter<remove_cvref_t<yielded>>{remove_cvref_t<yielded>(lval), this};
  }
  template <class R2, class V2, class Alloc2, class Unused>
    requires same_as<typename generator<R2, V2, Alloc2>::yielded, yielded>
  auto yield_value(ranges::elements_of<generator<R2, V2, Alloc2>&&, Unused> g) noexcept {
    return typename base::template nested_awaiter<generator<R2, V2, Alloc2>>{
        static_cast<generator<R2, V2, Alloc2>&&>(g.range)};
  }
  template <class R2, class V2, class Alloc2, class Unused>
    requires same_as<typename generator<R2, V2, Alloc2>::yielded, yielded>
  auto yield_value(ranges::elements_of<generator<R2, V2, Alloc2>&, Unused> g) noexcept {
    return typename base::template nested_awaiter<generator<R2, V2, Alloc2>>{
        static_cast<generator<R2, V2, Alloc2>&&>(g.range)};
  }
  template <ranges::input_range R, class Alloc>
    requires convertible_to<ranges::range_reference_t<R>, yielded>
  auto yield_value(ranges::elements_of<R, Alloc> r) {
    auto nested = [](allocator_arg_t, Alloc, ranges::iterator_t<R> i,
                     ranges::sentinel_t<R> s) -> generator<yielded, void, Alloc> {
      for (; i != s; ++i)
        co_yield static_cast<yielded>(*i);
    };
    return yield_value(
        ranges::elements_of(nested(allocator_arg, r.allocator, ranges::begin(r.range), ranges::end(r.range))));
  }

  void await_transform() = delete;
  void return_void() const noexcept {}
  void unhandled_exception() {
    if constexpr (ycxx::detail::cfg::exceptions) {
      if (this->parent_ == nullptr)
        throw;
      this->except_ = current_exception();
    }
  }

  void* operator new(size_t size)
    requires same_as<Allocator, void> || default_initializable<Allocator>
  {
    if constexpr (is_void_v<Allocator>)
      return allocate(allocator<void>(), size);
    else
      return allocate(Allocator(), size);
  }
  template <class Alloc, class... Args>
  void* operator new(size_t size, allocator_arg_t, const Alloc& alloc, const Args&...) {
    return allocate_with(alloc, size);
  }
  template <class This, class Alloc, class... Args>
  void* operator new(size_t size, const This&, allocator_arg_t, const Alloc& alloc, const Args&...) {
    return allocate_with(alloc, size);
  }
  void operator delete(void* pointer, size_t size) noexcept {
    if constexpr (is_void_v<Allocator>)
      ycxx::detail::gen_erased_deallocate(pointer, size);
    else
      frame<unit_alloc<Allocator>, false>::deallocate(pointer, size);
  }
};

template <class Ref, class Val, class Allocator>
class generator<Ref, Val, Allocator>::iterator {
  friend class generator;
  coroutine_handle<promise_type> coroutine_;

  explicit iterator(coroutine_handle<promise_type> h) noexcept : coroutine_(h) {}

public:
  using value_type = value;
  using difference_type = ptrdiff_t;

  iterator(iterator&& other) noexcept : coroutine_(other.coroutine_) { other.coroutine_ = nullptr; }
  iterator& operator=(iterator&& other) noexcept {
    coroutine_ = other.coroutine_;
    other.coroutine_ = nullptr;
    return *this;
  }
  reference operator*() const noexcept(is_nothrow_copy_constructible_v<reference>) {
    ycxx::detail::precondition(!coroutine_.done(), "std::generator::iterator::operator*: at the end");
    return static_cast<reference>(*coroutine_.promise().top_->value_);
  }
  iterator& operator++() {
    ycxx::detail::precondition(!coroutine_.done(), "std::generator::iterator::operator++: at the end");
    coroutine_.promise().top_->self_.resume();
    return *this;
  }
  void operator++(int) { ++*this; }
  friend bool operator==(const iterator& i, default_sentinel_t) { return i.coroutine_.done(); }
};

namespace pmr {
template <class Ref, class Val = void>
using generator = std::generator<Ref, Val, polymorphic_allocator<>>;
}

} // namespace std
