// [expr.new]/28: "If any part of the object initialization described above terminates by
// throwing an exception and a suitable deallocation function can be found, the deallocation
// function is called to free the memory in which the object was being constructed, after
// which the exception continues to propagate in the context of the new-expression."
// For placement new, the matching placement deallocation function is called.
#include <cstddef>
#include <new>
#include "check.hpp"

static int allocs = 0, deallocs = 0, placement_deallocs = 0, dtors = 0;

struct Thrower {
  Thrower(int v) {
    if (v) throw v;
  }
  ~Thrower() { ++dtors; }
  static void* operator new(std::size_t sz) {
    ++allocs;
    return ::operator new(sz);
  }
  static void operator delete(void* p) {
    ++deallocs;
    ::operator delete(p);
  }
  static void* operator new(std::size_t sz, int tag) {
    CHECK(tag == 42);
    ++allocs;
    return ::operator new(sz);
  }
  static void operator delete(void* p, int tag) {
    CHECK(tag == 42);
    ++placement_deallocs;
    ::operator delete(p);
  }
};

struct Member {
  Thrower t;
  Member() : t(5) {}
};

int main() {
  int which = 0;
  try {
    (void)new Thrower(1);
  } catch (int i) {
    which = i;
  }
  CHECK(which == 1 && allocs == 1 && deallocs == 1 && dtors == 0);

  which = 0;
  try {
    (void)new (42) Thrower(2);
  } catch (int i) {
    which = i;
  }
  CHECK(which == 2 && allocs == 2 && placement_deallocs == 1 && deallocs == 1);

  // success path: no deallocation until delete
  Thrower* p = new Thrower(0);
  CHECK(allocs == 3 && deallocs == 1);
  delete p;
  CHECK(deallocs == 2 && dtors == 1);

  // global operator new with a throwing member: the global storage is released (observed
  // indirectly: no leak checks here, just that the exception propagates)
  which = 0;
  try {
    (void)new Member;
  } catch (int i) {
    which = i;
  }
  CHECK(which == 5);
  return 0;
}
