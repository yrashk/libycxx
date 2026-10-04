// [util.smartptr.shared.create]/7 for make_shared_for_overwrite and
// allocate_shared_for_overwrite with array types:
// (7.8) each non-array subobject is "initialized via the expression ::new(pv) U" (so never
//   through the allocator's construct, even for allocate_shared_for_overwrite);
// (7.9) "Array elements are initialized in ascending order of their addresses."
// (7.10) "When the lifetime of the object managed by the return value ends, or when the
//   initialization of an array element throws an exception, the initialized elements are
//   destroyed in the reverse order of their original construction."
// (7.11) they are destroyed via pu->~U() (never the allocator's destroy).
// /3: the allocate_ form takes its memory from a rebound copy of the allocator, and on an
// exception has no effect (memory released).
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
  // _for_overwrite: ::new(pv) U and pu->~U(), never the allocator's construct/destroy
  counts = Counts{};
  reset();
  {
    auto p = std::allocate_shared_for_overwrite<Tracker[]>(RecAlloc<int>(), 4);
    CHECK(nctor == 4 && counts.allocs == 1);
  }
  CHECK(counts.constructs == 0 && counts.destroys == 0 && counts.outstanding == 0);
  CHECK(ascending_then_reverse(4));
  counts = Counts{};
  reset();
  {
    auto p = std::allocate_shared_for_overwrite<Tracker[2][2]>(RecAlloc<int>());
  }
  CHECK(counts.constructs == 0 && counts.destroys == 0 && counts.outstanding == 0);
  CHECK(ascending_then_reverse(4));
  reset();
  {
    auto p = std::make_shared_for_overwrite<Tracker[]>(3);
    auto q = std::make_shared_for_overwrite<Tracker[3]>();
  }
  CHECK(nctor == 6 && ndtor == 6);
  CHECK(dtor_addr[0] == ctor_addr[5] && dtor_addr[2] == ctor_addr[3]);  // q first, reversed
  CHECK(dtor_addr[3] == ctor_addr[2] && dtor_addr[5] == ctor_addr[0]);  // then p, reversed
  counts = Counts{};
  reset();
  throw_on = 2;
  CHECK(throws_boom([] { (void)std::allocate_shared_for_overwrite<Tracker[]>(RecAlloc<int>(), 4); }));
  CHECK(nctor == 2 && ndtor == 2 && dtor_addr[0] == ctor_addr[1] && counts.outstanding == 0);
  return 0;
}
