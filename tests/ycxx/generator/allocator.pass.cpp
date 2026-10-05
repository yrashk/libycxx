// [coro.generator.promise]/17-22: the coroutine state of generator<Ref, Val, Allocator> is
// allocated with
//   - Allocator, if it is not void (default-constructed unless an allocator_arg_t, alloc pair
//     is the first parameter or follows the object parameter, then A(alloc));
//   - Alloc, the type of the allocator after allocator_arg, when Allocator is void;
//   - allocator<void> otherwise;
// rebound to an unspecified type U, and deallocated with an allocator equal to it.
// The overload taking (const This&, allocator_arg_t, const Alloc&, ...) serves member functions.
// [generator.syn]: pmr::generator uses polymorphic_allocator<>.
// REQUIRES: exceptions
#include <generator>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <vector>
#include "check.hpp"
#include "recording_resource.hpp"

struct Stats {
  int allocs = 0, deallocs = 0;
  long outstanding = 0;
};

template <class T>
struct StatAlloc {
  using value_type = T;
  Stats* s;
  explicit StatAlloc(Stats* p) noexcept : s(p) {}
  template <class U>
  StatAlloc(const StatAlloc<U>& o) noexcept : s(o.s) {}
  T* allocate(std::size_t n) {
    ++s->allocs;
    s->outstanding += static_cast<long>(n * sizeof(T));
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    ++s->deallocs;
    s->outstanding -= static_cast<long>(n * sizeof(T));
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U>
  friend bool operator==(const StatAlloc& a, const StatAlloc<U>& b) noexcept { return a.s == b.s; }
};
static_assert(!std::is_default_constructible_v<StatAlloc<int>>);

Stats default_stats;
template <class T>
struct DefaultAlloc {
  using value_type = T;
  DefaultAlloc() = default;
  template <class U>
  DefaultAlloc(const DefaultAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++default_stats.allocs;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    ++default_stats.deallocs;
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U>
  friend bool operator==(const DefaultAlloc&, const DefaultAlloc<U>&) noexcept { return true; }
};

// Allocator void: the allocator after allocator_arg is used.
std::generator<int> any_alloc(std::allocator_arg_t, StatAlloc<char>, int n) {
  for (int i = 0; i < n; ++i) co_yield i;
}
// Allocator given: the allocator argument converts to it.
std::generator<int, void, StatAlloc<int>> fixed_alloc(std::allocator_arg_t, const StatAlloc<long>&, int n) {
  for (int i = 0; i < n; ++i) co_yield i;
}
// Allocator given and default-constructible: no allocator argument needed.
std::generator<int, void, DefaultAlloc<int>> default_alloc(int n) {
  for (int i = 0; i < n; ++i) co_yield i;
}

struct Obj {
  int base;
  std::generator<int> member(std::allocator_arg_t, StatAlloc<int>, int n) const {
    for (int i = 0; i < n; ++i) co_yield base + i;
  }
};

std::pmr::generator<int> pmr_gen(std::allocator_arg_t, std::pmr::polymorphic_allocator<> a, int n) {
  for (int i = 0; i < n; ++i) co_yield i;
  (void)a;
}

// Nested generators with their own allocators.
std::generator<int> nested(std::allocator_arg_t, StatAlloc<int> a) {
  co_yield std::ranges::elements_of(any_alloc(std::allocator_arg, StatAlloc<char>(a), 2));
  co_yield 9;
}

template <class G>
int sum(G&& g) {
  int s = 0;
  for (int x : g) s += x;
  return s;
}

int main() {
  {
    Stats st;
    {
      auto g = any_alloc(std::allocator_arg, StatAlloc<char>(&st), 4);
      CHECK(st.allocs == 1 && st.deallocs == 0 && st.outstanding > 0);
      CHECK(sum(g) == 6);
    }
    CHECK(st.allocs == 1 && st.deallocs == 1 && st.outstanding == 0);
  }
  {
    Stats st;
    {
      auto g = fixed_alloc(std::allocator_arg, StatAlloc<long>(&st), 3);
      CHECK(st.allocs == 1);
      CHECK(sum(g) == 3);
    }
    CHECK(st.deallocs == 1 && st.outstanding == 0);
  }
  {
    default_stats = {};
    {
      auto g = default_alloc(3);
      CHECK(default_stats.allocs == 1);
      CHECK(sum(g) == 3);
    }
    CHECK(default_stats.deallocs == 1);
  }
  {
    Stats st;
    Obj o{10};
    {
      auto g = o.member(std::allocator_arg, StatAlloc<int>(&st), 2);
      CHECK(st.allocs == 1);
      CHECK(sum(g) == 21);
    }
    CHECK(st.deallocs == 1 && st.outstanding == 0);
  }
  {
    RecordingResource r;
    {
      auto g = pmr_gen(std::allocator_arg, &r, 3);
      CHECK(r.allocs == 1);
      CHECK(sum(g) == 3);
    }
    CHECK(r.deallocs == 1 && r.outstanding == 0);
  }
  {
    Stats st;
    {
      auto g = nested(std::allocator_arg, StatAlloc<int>(&st));
      CHECK(st.allocs == 1);
      CHECK(sum(g) == 10);
      CHECK(st.allocs == 2);
    }
    CHECK(st.deallocs == 2 && st.outstanding == 0);
  }
  return 0;
}
