// Allocators for libycxx's own suite, written from [allocator.requirements] and
// [container.alloc.reqmts]. Deliberately independent of every other test suite.
#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>

// Stateful allocator; two allocators compare equal iff their ids are equal.
// The propagate_on_container_* traits are chosen by the template arguments.
template <class T, bool CopyProp = false, bool MoveProp = false, bool SwapProp = false>
struct IdAlloc {
  using value_type = T;
  using propagate_on_container_copy_assignment = std::bool_constant<CopyProp>;
  using propagate_on_container_move_assignment = std::bool_constant<MoveProp>;
  using propagate_on_container_swap = std::bool_constant<SwapProp>;
  using is_always_equal = std::false_type;
  template <class U>
  struct rebind {
    using other = IdAlloc<U, CopyProp, MoveProp, SwapProp>;
  };
  int id = 0;
  constexpr IdAlloc() noexcept = default;
  constexpr explicit IdAlloc(int i) noexcept : id(i) {}
  template <class U>
  constexpr IdAlloc(const IdAlloc<U, CopyProp, MoveProp, SwapProp>& o) noexcept : id(o.id) {}
  constexpr T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  constexpr void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend constexpr bool operator==(const IdAlloc& a, const IdAlloc<U, CopyProp, MoveProp, SwapProp>& b) noexcept {
    return a.id == b.id;
  }
};

// Global counters for CountingAlloc / LimitedAlloc.
struct AllocCounters {
  int allocations = 0;
  int deallocations = 0;
  int constructs = 0;
  int destroys = 0;
  long outstanding = 0;  // allocated minus deallocated element count
  int fail_after = -1;   // allocate() throws bad_alloc once this reaches 0; -1 = never
};
inline AllocCounters alloc_counters;

// Allocator that counts allocate/deallocate/construct/destroy calls and can be armed to
// throw from allocate(). Always equal.
template <class T>
struct CountingAlloc {
  using value_type = T;
  CountingAlloc() = default;
  template <class U>
  CountingAlloc(const CountingAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    if (alloc_counters.fail_after == 0) throw std::bad_alloc();
    if (alloc_counters.fail_after > 0) --alloc_counters.fail_after;
    ++alloc_counters.allocations;
    alloc_counters.outstanding += static_cast<long>(n);
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) {
    ++alloc_counters.deallocations;
    alloc_counters.outstanding -= static_cast<long>(n);
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ++alloc_counters.constructs;
    ::new (static_cast<void*>(p)) U(static_cast<Args&&>(args)...);
  }
  template <class U>
  void destroy(U* p) {
    ++alloc_counters.destroys;
    p->~U();
  }
  friend bool operator==(const CountingAlloc&, const CountingAlloc&) noexcept { return true; }
};

// The smallest allocator [allocator.requirements.general] permits: value_type, allocate,
// deallocate, a converting constructor and ==. Not default-constructible on purpose is not
// done here, since containers' default constructors need it.
template <class T>
struct MinimalAlloc {
  using value_type = T;
  MinimalAlloc() = default;
  template <class U>
  constexpr MinimalAlloc(const MinimalAlloc<U>&) noexcept {}
  constexpr T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  constexpr void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend constexpr bool operator==(const MinimalAlloc&, const MinimalAlloc<U>&) noexcept { return true; }
};
