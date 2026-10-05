// [container.alloc.reqmts]/2, Note 2: "A container calls allocator_traits<A>::construct(m, p,
// args) to construct an element at p using args, with m == get_allocator()"; elements are
// destroyed with allocator_traits<A>::destroy(m, p) ([container.alloc.reqmts]/2.7). This holds
// for trivially copyable element types too, so when the allocator provides construct and
// destroy, a container may not create elements by copying bytes (for instance when relocating
// on reallocation or shifting on insertion): every object the container holds must have been
// constructed by the allocator at its own address. An allocator that records the addresses it
// constructed and destroyed checks, after every operation of vector and deque on int, char and
// a small aggregate, that the recorded live addresses are exactly the element addresses.
#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>
#include "check.hpp"

static std::vector<const void*>* live;  // addresses constructed and not yet destroyed
static long constructs = 0;

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
    ++constructs;
    live->push_back(p);
  }
  template <class U>
  void destroy(U* p) {
    auto it = std::find(live->begin(), live->end(), static_cast<const void*>(p));
    CHECK(it != live->end());  // only what was constructed is destroyed
    live->erase(it);
    p->~U();
  }
  friend bool operator==(const TrackAlloc&, const TrackAlloc&) { return true; }
};

struct Pod {
  int a;
  short b;
  Pod(int x = 0) : a(x), b(static_cast<short>(-x)) {}
};
static int val(int x) { return x; }
static int val(char x) { return x; }
static int val(const Pod& p) { return p.a; }

template <class C>
void verify(const C& c) {
  std::vector<const void*> addrs;
  for (const auto& e : c) addrs.push_back(&e);
  std::vector<const void*> l = *live;
  std::sort(addrs.begin(), addrs.end());
  std::sort(l.begin(), l.end());
  CHECK(l == addrs);
}

template <class C>
void run() {
  using T = typename C::value_type;
  std::vector<const void*> store;
  live = &store;
  {
    C c;
    for (int i = 0; i < 100; ++i) {
      if (i % 3 == 0) c.push_back(T(i));
      else c.emplace_back(i);
      verify(c);
    }
    for (int i = 0; i < 20; ++i) {
      c.insert(c.begin() + (i * 7) % static_cast<int>(c.size()), T(1000 + i));
      verify(c);
      c.insert(c.begin() + (i * 5) % static_cast<int>(c.size()), 3, *(c.begin() + i));  // aliasing value
      verify(c);
      c.emplace(c.begin() + (i * 11) % static_cast<int>(c.size()), 2000 + i);
      verify(c);
    }
    T src[7] = {T(1), T(2), T(3), T(4), T(5), T(6), T(7)};
    for (int i = 0; i < 10; ++i) {
      c.insert(c.begin() + (i * 13) % static_cast<int>(c.size()), src, src + i % 8);
      verify(c);
    }
    for (int i = 0; i < 30; ++i) {
      c.erase(c.begin() + (i * 17) % static_cast<int>(c.size()));
      verify(c);
      auto f = c.begin() + (i * 3) % static_cast<int>(c.size() - 5);
      c.erase(f, f + i % 5);
      verify(c);
    }
    if constexpr (requires { c.reserve(1); }) {
      c.reserve(c.capacity() * 2 + 10);
      verify(c);
    }
    c.shrink_to_fit();
    verify(c);
    c.resize(c.size() + 50);
    verify(c);
    c.resize(c.size() - 70);
    verify(c);
    if constexpr (requires { c.push_front(T(1)); }) {
      for (int i = 0; i < 60; ++i) {
        c.push_front(T(-i));
        verify(c);
      }
      for (int i = 0; i < 30; ++i) {
        c.pop_front();
        verify(c);
      }
    }
    C d = c;
    {
      std::vector<const void*> all;
      for (const auto& e : c) all.push_back(&e);
      for (const auto& e : d) all.push_back(&e);
      std::vector<const void*> l = *live;
      std::sort(all.begin(), all.end());
      std::sort(l.begin(), l.end());
      CHECK(l == all);
    }
    d.assign(40, T(9));
    c = d;
    C e = std::move(d);
    {
      std::vector<const void*> all;
      for (const auto& x : c) all.push_back(&x);
      for (const auto& x : d) all.push_back(&x);
      for (const auto& x : e) all.push_back(&x);
      std::vector<const void*> l = *live;
      std::sort(all.begin(), all.end());
      std::sort(l.begin(), l.end());
      CHECK(l == all);
    }
    for (const auto& x : c) CHECK(val(x) == val(T(9)));
  }
  CHECK(live->empty());
  live = nullptr;
}

int main() {
  run<std::vector<int, TrackAlloc<int>>>();
  run<std::vector<char, TrackAlloc<char>>>();
  run<std::vector<Pod, TrackAlloc<Pod>>>();
  run<std::deque<int, TrackAlloc<int>>>();
  run<std::deque<Pod, TrackAlloc<Pod>>>();
  CHECK(constructs > 1000);
}
