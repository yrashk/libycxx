// basic_stacktrace with an allocator whose allocate() throws.
// [stacktrace.basic.cons]/1, /3, /6: current(alloc), current(skip, alloc) and
// current(skip, max_depth, alloc) are noexcept and return "an empty basic_stacktrace object if
// the initialization of frames_ failed"; alloc is passed to the constructor of frames_ (so the
// result's get_allocator() compares equal to alloc, [stacktrace.basic.obs]: get_allocator()
// returns frames_.get_allocator()). /7-9: the default and allocator constructors give empty
// stacktraces. /10: copying may throw (or, as a strengthening, yield an empty stacktrace).
// An allocator that fails only after some successful allocations exercises a failure in the
// middle of building frames_.
#include <cstddef>
#include <memory>
#include <new>
#include <stacktrace>
#include <utility>
#include "check.hpp"

int budget = -1;  // allocations allowed before throwing; -1: unlimited
int live = 0;

template <class T>
struct FailAlloc {
  using value_type = T;
  int id = 0;
  FailAlloc() = default;
  explicit FailAlloc(int i) : id(i) {}
  template <class U>
  FailAlloc(const FailAlloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) {
    if (budget == 0) throw std::bad_alloc();
    if (budget > 0) --budget;
    ++live;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    --live;
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U>
  friend bool operator==(const FailAlloc& a, const FailAlloc<U>& b) noexcept {
    return a.id == b.id;
  }
};

using ST = std::basic_stacktrace<FailAlloc<std::stacktrace_entry>>;
static_assert(noexcept(ST::current()));
static_assert(noexcept(ST::current(std::declval<const FailAlloc<std::stacktrace_entry>&>())));
static_assert(noexcept(ST::current(1, 2)));

[[gnu::noinline]] ST deep(int n, const FailAlloc<std::stacktrace_entry>& a) {
  if (n == 0)
    return ST::current(a);
  ST r = deep(n - 1, a);
  return r;
}

void run() {
  const FailAlloc<std::stacktrace_entry> a(7);
  budget = 0;
  ST t = ST::current(a);
  CHECK(t.empty() && t.size() == 0 && t.begin() == t.end());
  CHECK(ST::current(1, a).empty());
  CHECK(ST::current(0, 5, a).empty());
  CHECK(deep(20, a).empty());
  budget = -1;
  ST d(a);
  CHECK(d.empty() && d.get_allocator() == a);

  // Successful captures use (a copy of) the given allocator.
  ST ok = ST::current(a);
  CHECK(ok.get_allocator() == a);
  if (!ok.empty()) {
    // A failure after one allocation: still empty (or complete), never a partial result
    // that differs from the full one at its start.
    for (int k = 1; k <= 3; ++k) {
      budget = k;
      ST p = deep(10, a);
      budget = -1;
      ST full = deep(10, a);
      CHECK(p.empty() || p.size() == full.size());
    }
    // Copying with a failing allocator throws bad_alloc or yields an empty stacktrace.
    budget = 0;
    bool threw = false;
    try {
      ST c(ok);
      CHECK(c.empty());
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    budget = -1;
    (void)threw;
    CHECK(!ok.empty());
  }
}

int main() {
  run();
  CHECK(live == 0);  // everything allocated was deallocated
  return 0;
}
