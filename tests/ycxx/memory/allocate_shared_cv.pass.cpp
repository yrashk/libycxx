// [util.smartptr.shared.create]/7.5, 7.7, 7.12: allocate_shared initializes a (sub)object of
// non-array type U "via the expression allocator_traits<A2>::construct(a2, pu, ...) ... where
// pu is a pointer of type remove_cv_t<U>* pointing to storage suitable to hold an object of type
// remove_cv_t<U>", and destroys it with allocator_traits<A2>::destroy(a2, pu), same pu type.
// /3: the allocator is "a copy of a (rebound for an unspecified value_type)"; an allocator's
// value_type is a cv-unqualified object type ([allocator.requirements.general]/2), so the
// rebound copy cannot be one for const U. Checked for allocate_shared<const T>,
// allocate_shared<const T[N]> and allocate_shared<const T[]> with an allocator that only
// works for cv-unqualified value types (like std::allocator).
// REQUIRES: exceptions
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

int main() {
  counts = Counts{};
  reset();
  {
    auto p = std::allocate_shared<const Tracker>(RecAlloc<int>(), 5);
    CHECK(p->v == 5);
  }
  CHECK(counts.constructs == 1 && counts.destroys == 1 && !counts.construct_on_non_tracker);
  CHECK(counts.outstanding == 0);

  counts = Counts{};
  reset();
  {
    auto p = std::allocate_shared<const Tracker[4]>(RecAlloc<int>());
    CHECK(p[3].v == -1);
  }
  CHECK(counts.constructs == 4 && counts.destroys == 4 && !counts.construct_on_non_tracker);
  CHECK(ascending_then_reverse(4));

  counts = Counts{};
  reset();
  {
    auto p = std::allocate_shared<const Tracker[]>(RecAlloc<int>(), 3, Tracker(8));
    CHECK(p[2].v == 8);
  }
  CHECK(counts.constructs == 3 && counts.destroys == 3 && !counts.construct_on_non_tracker);
  CHECK(counts.outstanding == 0);
  return 0;
}
