// [util.smartptr.shared.create]/7.10: "When ... the initialization of an array element throws
// an exception, the initialized elements are destroyed in the reverse order of their original
// construction." /7.12: for allocate_shared through allocator_traits<A2>::destroy. /3: "If an exception is thrown, the functions have no
// effect" (the memory obtained is released); /6: the exception thrown from the initialization
// of the object propagates. /7.9: elements are initialized in ascending address order.
#include <memory>
#include <cstddef>
#include <new>
#include <type_traits>
#include "check.hpp"

struct Boom {};

// Global event log: the addresses of constructed and destroyed Trackers, in order.
const void* ctor_addr[64];
const void* dtor_addr[64];
int nctor = 0, ndtor = 0, throw_on = -1;

struct Tracker {
  int v;
  Tracker() : v(-1) { record(); }
  Tracker(int x) : v(x) { record(); }
  Tracker(const Tracker& o) : v(o.v) { record(); }
  ~Tracker() { dtor_addr[ndtor++] = this; }
  void record() {
    if (nctor == throw_on) throw Boom{};
    ctor_addr[nctor++] = this;
  }
};

void reset() { nctor = ndtor = 0, throw_on = -1; }

// constructed at strictly ascending addresses, destroyed in exactly the reverse order
bool ascending_then_reverse(int n) {
  if (nctor != n || ndtor != n) return false;
  for (int i = 1; i < n; ++i)
    if (!(static_cast<const char*>(ctor_addr[i - 1]) < static_cast<const char*>(ctor_addr[i]))) return false;
  for (int i = 0; i < n; ++i)
    if (dtor_addr[i] != ctor_addr[n - 1 - i]) return false;
  return true;
}

// Allocator recording construct/destroy calls and the pointer types they receive.
struct Counts {
  int allocs = 0, deallocs = 0, constructs = 0, destroys = 0;
  bool construct_on_non_tracker = false;
  long outstanding = 0;
};
Counts counts;

template <class T>
struct RecAlloc {
  using value_type = T;
  RecAlloc() = default;
  template <class U>
  RecAlloc(const RecAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++counts.allocs;
    counts.outstanding += static_cast<long>(n * sizeof(T));
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) {
    ++counts.deallocs;
    counts.outstanding -= static_cast<long>(n * sizeof(T));
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ++counts.constructs;
    if (!std::is_same_v<U, Tracker>) counts.construct_on_non_tracker = true;
    ::new (static_cast<void*>(p)) U(static_cast<Args&&>(args)...);
  }
  template <class U>
  void destroy(U* p) {
    ++counts.destroys;
    p->~U();
  }
  friend bool operator==(const RecAlloc&, const RecAlloc&) noexcept { return true; }
};

template <class F>
bool throws_boom(F f) {
  try {
    f();
  } catch (const Boom&) {
    return true;
  }
  return false;
}

int main() {
  // an element's initialization throws: the constructed ones are destroyed in reverse order
  for (int k = 0; k < 6; ++k) {
    reset();
    throw_on = k;
    CHECK(throws_boom([] { (void)std::make_shared<Tracker[]>(6); }));
    CHECK(nctor == k && ndtor == k);
    for (int i = 0; i < k; ++i) CHECK(dtor_addr[i] == ctor_addr[k - 1 - i]);
  }
  reset();
  throw_on = 4;
  CHECK(throws_boom([] { (void)std::make_shared<Tracker[3][2]>(); }));
  CHECK(nctor == 4 && ndtor == 4 && dtor_addr[0] == ctor_addr[3] && dtor_addr[3] == ctor_addr[0]);

  counts = Counts{};
  for (int k = 0; k < 5; ++k) {
    counts = Counts{};
    reset();
    throw_on = k;
    CHECK(throws_boom([] { (void)std::allocate_shared<Tracker[]>(RecAlloc<int>(), 5, Tracker(3)); }));
    // with k == 0 the temporary Tracker(3) throws before the call; nothing was allocated
    CHECK(counts.destroys == (k == 0 ? 0 : k - 1));
    CHECK(counts.outstanding == 0);
    if (k > 0) {
      // the temporary is construction 0; elements 1..k-1 were constructed and are destroyed in
      // reverse (then the temporary is destroyed as the full-expression ends)
      CHECK(nctor == k && ndtor == k);
      for (int i = 0; i + 1 < k; ++i) CHECK(dtor_addr[i] == ctor_addr[k - 1 - i]);
    }
  }

  return 0;
}
