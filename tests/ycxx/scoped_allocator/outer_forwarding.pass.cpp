// [allocator.adaptor.members]/5-8: allocate(n) returns allocator_traits<OuterAlloc>::
// allocate(outer_allocator(), n); allocate(n, hint) returns allocator_traits<OuterAlloc>::
// allocate(outer_allocator(), n, hint) -- which calls the outer allocator's allocate(n, hint)
// if it has one and allocate(n) otherwise ([allocator.traits.members]/2); deallocate forwards
// to the outer allocator; max_size() is allocator_traits<OuterAlloc>::max_size(
// outer_allocator()) (the member if present). The inner allocators are never asked for
// memory. All of this goes to the outer allocator object stored in the adaptor (state
// preserved, not a copy).
#include <scoped_allocator>
#include <cstddef>
#include <memory>
#include <type_traits>
#include "check.hpp"

struct Log {
  int alloc = 0, alloc_hint = 0, dealloc = 0;
  const void* last_hint = nullptr;
};

template <class T, bool Hint>
struct Outer {
  using value_type = T;
  template <class U>
  struct rebind { using other = Outer<U, Hint>; };
  Log* log;
  explicit Outer(Log* l) : log(l) {}
  template <class U>
  Outer(const Outer<U, Hint>& o) : log(o.log) {}
  T* allocate(std::size_t n) {
    ++log->alloc;
    return std::allocator<T>{}.allocate(n);
  }
  T* allocate(std::size_t n, const void* hint)
    requires Hint
  {
    ++log->alloc_hint;
    log->last_hint = hint;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) {
    ++log->dealloc;
    std::allocator<T>{}.deallocate(p, n);
  }
  std::size_t max_size() const { return 12345; }
  template <class U>
  friend bool operator==(const Outer& a, const Outer<U, Hint>& b) { return a.log == b.log; }
};

template <class T>
struct Inner {  // must never allocate
  using value_type = T;
  Inner() = default;
  template <class U>
  Inner(const Inner<U>&) {}
  T* allocate(std::size_t) {
    CHECK(false);
    return nullptr;
  }
  void deallocate(T*, std::size_t) { CHECK(false); }
  template <class U>
  friend bool operator==(const Inner&, const Inner<U>&) { return true; }
};

template <bool Hint>
void run() {
  Log log, inner_log;
  using S = std::scoped_allocator_adaptor<Outer<int, Hint>, Inner<int>, Outer<int, !Hint>>;
  S s{Outer<int, Hint>(&log), Inner<int>(), Outer<int, !Hint>(&inner_log)};
  int* p = s.allocate(3);
  CHECK(log.alloc == 1 && log.alloc_hint == 0);
  int* q = s.allocate(2, p);
  if constexpr (Hint)
    CHECK(log.alloc == 1 && log.alloc_hint == 1 && log.last_hint == p);
  else
    CHECK(log.alloc == 2 && log.alloc_hint == 0);
  std::allocator_traits<S>::deallocate(s, q, 2);
  s.deallocate(p, 3);
  CHECK(log.dealloc == 2);
  static_assert(noexcept(s.deallocate(p, 3)));
  CHECK(s.max_size() == 12345 && std::allocator_traits<S>::max_size(s) == 12345);
  int* r = std::allocator_traits<S>::allocate(s, 1, nullptr);
  CHECK(log.alloc + log.alloc_hint == 3);
  s.deallocate(r, 1);
  CHECK(inner_log.alloc == 0 && inner_log.alloc_hint == 0 && inner_log.dealloc == 0);
}

int main() {
  run<true>();
  run<false>();
  return 0;
}
