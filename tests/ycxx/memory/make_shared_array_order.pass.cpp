// [util.smartptr.shared.create]/7 for the array forms (cv-qualified element types are in
// allocate_shared_cv.pass.cpp):
// (7.4)/(7.6) make_shared initializes each non-array subobject with ::new(pv) U(...) / U();
// (7.5)/(7.7) allocate_shared with allocator_traits<A2>::construct(a2, pu, ...), pu of type
//   remove_cv_t<U>*, a2 a rebound copy of a;
// (the _for_overwrite forms are in make_shared_for_overwrite_order.pass.cpp);
// (7.9) "Array elements are initialized in ascending order of their addresses."
// (7.10) "When the lifetime of the object managed by the return value ends, ... the initialized
//   elements are destroyed in the reverse order of their original construction." (The case of
//   a throwing element initialization is in make_shared_array_throw.pass.cpp.)
// (7.11) objects from make_shared are destroyed with pu->~U(); (7.12) objects from
//   allocate_shared with allocator_traits<A2>::destroy(a2, pu).
// Multidimensional arrays are initialized element by element of the innermost non-array type
// (7.2-7.3 by recursion).
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
  // make_shared<T[]>(N): value-initialized (default constructor), ascending, reverse
  reset();
  {
    auto p = std::make_shared<Tracker[]>(5);
    CHECK(p.use_count() == 1 && p[0].v == -1 && p[4].v == -1);
    CHECK(nctor == 5 && ndtor == 0);
    CHECK(ctor_addr[0] == &p[0] && ctor_addr[4] == &p[4]);
  }
  CHECK(ascending_then_reverse(5));

  // make_shared<T[N]>(u) / make_shared<T[]>(N, u): each element from u
  reset();
  {
    auto p = std::make_shared<Tracker[3]>(Tracker(7));
    CHECK(p[2].v == 7);
  }
  CHECK(nctor == 4 && ndtor == 4);  // the temporary and the three elements
  reset();
  {
    auto p = std::make_shared<Tracker[]>(4, Tracker(9));
    CHECK(p[3].v == 9);
    nctor = ndtor = 0;
  }
  CHECK(ndtor == 4);

  // multidimensional: T[][2] with an initial value u of type T[2]: each element of each row
  // from the corresponding element of u
  reset();
  {
    Tracker u[2] = {Tracker(1), Tracker(2)};
    reset();
    {
      auto p = std::make_shared<Tracker[][2]>(3, u);
      CHECK(p[0][0].v == 1 && p[0][1].v == 2 && p[2][0].v == 1 && p[2][1].v == 2);
      CHECK(ctor_addr[0] == &p[0][0] && ctor_addr[5] == &p[2][1]);
    }
    CHECK(ascending_then_reverse(6));
  }
  reset();
  {
    auto p = std::make_shared<Tracker[2][3]>();
    CHECK(p[1][2].v == -1);
  }
  CHECK(ascending_then_reverse(6));

  // allocate_shared: through allocator_traits<A2>::construct/destroy on Tracker* (non-array,
  // cv removed); memory from the allocator, released afterwards and on failure
  counts = Counts{};
  reset();
  {
    auto p = std::allocate_shared<Tracker[][2]>(RecAlloc<int>(), 3);
    CHECK(counts.constructs == 6 && counts.destroys == 0 && counts.allocs == 1);
  }
  CHECK(counts.destroys == 6 && counts.outstanding == 0 && counts.deallocs == counts.allocs);
  CHECK(!counts.construct_on_non_tracker);
  CHECK(ascending_then_reverse(6));
  return 0;
}
