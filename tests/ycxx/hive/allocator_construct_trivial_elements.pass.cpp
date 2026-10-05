// [hive.overview]/6: a hive meets the requirements of an allocator-aware container
// ([container.alloc.reqmts]); /2 Note 2 there: "A container calls allocator_traits<A>::construct(m,
// p, args) to construct an element at p using args", and elements are destroyed with
// allocator_traits<A>::destroy ([container.alloc.reqmts]/2.7). This holds for trivially
// copyable element types too: when the allocator provides construct and destroy, a hive may not
// create elements by copying bytes, also not when it relocates them (shrink_to_fit "may
// reallocate elements" and reshape reallocates the elements of blocks outside the new limits,
// both with the precondition "T is Cpp17MoveInsertable into hive", [hive.capacity]), nor when
// sorting ([hive.operations]: sort requires Cpp17MoveInsertable) or copying the container.
// A tracking allocator records the addresses of the element objects it constructed (calls of
// construct for the element type; the hive may construct other internal objects through
// rebound copies) and checks, after every operation, that the recorded live addresses are
// exactly the addresses of the elements.
#include <algorithm>
#include <cstddef>
#include <hive>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>
#include "check.hpp"

struct Pod {
  int a;
  short b;
  Pod(int x = 0) : a(x), b(static_cast<short>(-x)) {}
};
struct Byte {
  unsigned char c;
  Byte(int x = 0) : c(static_cast<unsigned char>(x)) {}
};
static_assert(std::is_trivially_copyable_v<Pod> && std::is_trivially_copyable_v<Byte>);
static int val(const Pod& p) { return p.a; }
static int val(const Byte& b) { return b.c; }

static std::vector<const void*>* live;  // element addresses constructed and not yet destroyed
static long element_constructs = 0;

template <class T>
struct TrackAlloc {
  using value_type = T;
  TrackAlloc() = default;
  template <class U>
  TrackAlloc(const TrackAlloc<U>&) {}
  T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (static_cast<void*>(p)) U(static_cast<Args&&>(args)...);
    if constexpr (std::is_same_v<U, Pod> || std::is_same_v<U, Byte>) {
      ++element_constructs;
      live->push_back(p);
    }
  }
  template <class U>
  void destroy(U* p) {
    if constexpr (std::is_same_v<U, Pod> || std::is_same_v<U, Byte>) {
      auto it = std::find(live->begin(), live->end(), static_cast<const void*>(p));
      CHECK(it != live->end());  // only what was constructed is destroyed
      live->erase(it);
    }
    p->~U();
  }
  friend bool operator==(const TrackAlloc&, const TrackAlloc&) { return true; }
};

template <class... C>
void verify(const C&... c) {
  std::vector<const void*> addrs;
  ((void)[&] {
    for (const auto& e : c) addrs.push_back(&e);
  }(), ...);
  CHECK(addrs.size() == (c.size() + ...));
  std::vector<const void*> l = *live;
  std::sort(addrs.begin(), addrs.end());
  std::sort(l.begin(), l.end());
  CHECK(l == addrs);
}

template <class C>
long sum(const C& c) {
  long s = 0;
  for (const auto& e : c) s += val(e);
  return s;
}

template <class T>
void run() {
  using C = std::hive<T, TrackAlloc<T>>;
  std::vector<const void*> store;
  live = &store;
  {
    C c;
    for (int i = 0; i < 300; ++i) {
      if (i % 2 == 0) c.insert(T(i % 100));
      else c.emplace(i % 100);
      verify(c);
    }
    // erase every third element, leaving holes in every block
    int k = 0;
    for (auto it = c.begin(); it != c.end();) {
      if (k++ % 3 == 0) it = c.erase(it);
      else ++it;
    }
    verify(c);
    long s = sum(c);
    c.insert(5, T(7));  // reuses erased locations
    verify(c);
    s += 5 * 7;
    T src[40];
    for (int i = 0; i < 40; ++i) src[i] = T(i);
    c.insert(src, src + 40);
    verify(c);
    c.insert_range(std::vector<T>(src, src + 10));
    verify(c);
    s += 780 + 45;
    CHECK(sum(c) == s);
    c.shrink_to_fit();
    verify(c);
    CHECK(sum(c) == s);
    c.reshape(std::hive_limits(c.block_capacity_hard_limits().min, c.block_capacity_hard_limits().min));
    verify(c);
    CHECK(sum(c) == s);
    c.reshape(std::hive_limits(64, c.block_capacity_hard_limits().max));
    verify(c);
    c.reserve(c.size() + 200);
    verify(c);
    c.sort([](const T& x, const T& y) { return val(x) < val(y); });
    verify(c);
    CHECK(sum(c) == s);
    CHECK(std::is_sorted(c.begin(), c.end(), [](const T& x, const T& y) { return val(x) < val(y); }));
    c.unique([](const T& x, const T& y) { return val(x) == val(y); });
    verify(c);
    {
      C d(c);  // copy: copies of the elements, through the allocator
      CHECK(d.size() == c.size());
      verify(c, d);
      C e(std::move(d));
      verify(c, d, e);
      C f(e.block_capacity_limits());  // splice requires x's blocks within e's limits
      f.insert(3, T(1));
      e.splice(f);
      verify(c, e, f);
      e.trim_capacity();
      e.shrink_to_fit();
      verify(c, e);
      e.assign(src, src + 20);
      CHECK(e.size() == 20 && sum(e) == 190);
      verify(c, e);
      e.assign(50, T(2));
      CHECK(sum(e) == 100);
      verify(c, e);
      c = e;
      verify(c, e);
    }
    verify(c);
    CHECK(sum(c) == 100);
    c.clear();
    verify(c);
    CHECK(store.empty());
  }
  CHECK(store.empty());
  CHECK(element_constructs > 0);
}

int main() {
  run<Pod>();
  run<Byte>();
}
