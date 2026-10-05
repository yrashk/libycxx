// [util.smartptr.shared.const]/6: shared_ptr(Y* p): "If an exception is thrown, delete p is
// called when T is not an array type, delete[] p otherwise." /8: "Throws: bad_alloc". /11:
// shared_ptr(p, d [, a]): "If an exception is thrown, d(p) is called." /29: for
// shared_ptr(unique_ptr&&): "If an exception is thrown, the constructor has no effect."
// /25, /27: shared_ptr(const weak_ptr&) throws bad_weak_ptr when r.expired() and then has no
// effect. [util.smartptr.shared.create]: make_shared propagates exceptions from T's
// constructor without leaking. Allocation failure is injected by replacing ::operator new
// ([replacement.functions]) and through a throwing allocator.
// REQUIRES: exceptions
#include <cstdlib>
#include <memory>
#include <new>
#include "test_allocators.hpp"
#include "check.hpp"

static int fail_next_new = 0;  // when > 0, the next operator new call throws
static long outstanding = 0;

void* operator new(std::size_t n) {
  if (fail_next_new > 0) {
    fail_next_new = 0;
    throw std::bad_alloc();
  }
  void* p = std::malloc(n ? n : 1);
  if (!p) throw std::bad_alloc();
  ++outstanding;
  return p;
}
void operator delete(void* p) noexcept {
  if (p) {
    --outstanding;
    std::free(p);
  }
}
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }

struct Obj {
  static inline int live = 0;
  Obj() { ++live; }
  ~Obj() { --live; }
};
struct Arr {
  static inline int live = 0;
  Arr() { ++live; }
  ~Arr() { --live; }
};

struct Del {
  int* calls;
  void operator()(Obj* p) const {
    ++*calls;
    delete p;
  }
};

struct ThrowingCtor {
  static inline int live = 0;
  explicit ThrowingCtor(int v) {
    if (v) throw v;
    ++live;
  }
  ~ThrowingCtor() { --live; }
};

int main() {
  long base = outstanding;
  // shared_ptr(Y*): control block allocation fails -> delete p
  {
    Obj* p = new Obj;
    CHECK(Obj::live == 1);
    fail_next_new = 1;
    bool threw = false;
    try {
      std::shared_ptr<Obj> s(p);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    if (threw) CHECK(Obj::live == 0);  // p was deleted
    // (an implementation that does not allocate here cannot fail: then nothing to check)
    fail_next_new = 0;
  }
  CHECK(Obj::live == 0);
  CHECK(outstanding == base);

  // array form -> delete[] p
  {
    Arr* p = new Arr[3];
    CHECK(Arr::live == 3);
    fail_next_new = 1;
    try {
      std::shared_ptr<Arr[]> s(p);
    } catch (const std::bad_alloc&) {
    }
    fail_next_new = 0;
  }
  CHECK(Arr::live == 0);
  CHECK(outstanding == base);

  // shared_ptr(p, d, a) with an allocator that throws -> d(p) called
  {
    int calls = 0;
    Obj* p = new Obj;
    alloc_counters.fail_after = 0;
    bool threw = false;
    try {
      std::shared_ptr<Obj> s(p, Del{&calls}, CountingAlloc<Obj>());
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    alloc_counters.fail_after = -1;
    CHECK(threw);
    CHECK(calls == 1 && Obj::live == 0);
  }

  // shared_ptr(unique_ptr&&) failing: no effect, r still owns
  {
    std::unique_ptr<Obj> u(new Obj);
    Obj* raw = u.get();
    fail_next_new = 1;
    bool threw = false;
    try {
      std::shared_ptr<Obj> s(std::move(u));
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    fail_next_new = 0;
    if (threw) CHECK(u.get() == raw && Obj::live == 1);
  }
  CHECK(Obj::live == 0);

  // weak_ptr expired: bad_weak_ptr, no effect
  {
    std::weak_ptr<Obj> w;
    {
      auto s = std::make_shared<Obj>();
      w = s;
    }
    bool threw = false;
    try {
      std::shared_ptr<Obj> s(w);
    } catch (const std::bad_weak_ptr&) {
      threw = true;
    }
    CHECK(threw && w.expired());
  }

  // make_shared / allocate_shared: constructor throws, storage released
  {
    long before = outstanding;
    bool threw = false;
    try {
      (void)std::make_shared<ThrowingCtor>(1);
    } catch (int) {
      threw = true;
    }
    CHECK(threw && ThrowingCtor::live == 0 && outstanding == before);
    alloc_counters = AllocCounters{};
    threw = false;
    try {
      (void)std::allocate_shared<ThrowingCtor>(CountingAlloc<ThrowingCtor>(), 2);
    } catch (int) {
      threw = true;
    }
    CHECK(threw && alloc_counters.outstanding == 0);
    CHECK(alloc_counters.allocations == alloc_counters.deallocations);
  }
  CHECK(outstanding == base);
  return 0;
}
