// [except.ctor]/3 (3.6): for "default-initialization, value-initialization, or
// direct-initialization of an array", the elements whose initialization completed are
// destroyed, in reverse order, when a later element's constructor throws. Also applies to
// new[] (with the storage released, [expr.new]/28 / [except.ctor] Note 4).
// REQUIRES: exceptions
#include <cstddef>
#include <new>
#include "check.hpp"

static int log_[32];
static int n = 0;
static int counter = 0;
static int fail_at = -1;

struct Elem {
  int id;
  Elem() : id(counter++) {
    if (id == fail_at) throw id;
  }
  ~Elem() { log_[n++] = id; }
  static void* operator new[](std::size_t sz) {
    ++news;
    return ::operator new(sz);
  }
  static void operator delete[](void* p) {
    ++deletes;
    ::operator delete(p);
  }
  static int news, deletes;
};
int Elem::news = 0;
int Elem::deletes = 0;

int main() {
  counter = 0;
  fail_at = 3;
  try {
    Elem arr[6];
    (void)arr;
  } catch (int i) {
    CHECK(i == 3);
  }
  CHECK(n == 3 && log_[0] == 2 && log_[1] == 1 && log_[2] == 0);

  // multidimensional array
  counter = 0;
  fail_at = 4;
  n = 0;
  try {
    Elem arr[2][3];
    (void)arr;
  } catch (int) {
  }
  CHECK(n == 4);
  for (int i = 0; i < 4; ++i) CHECK(log_[i] == 3 - i);

  // new[]
  counter = 0;
  fail_at = 2;
  n = 0;
  try {
    Elem* p = new Elem[5];
    delete[] p;
  } catch (int) {
  }
  CHECK(n == 2 && log_[0] == 1 && log_[1] == 0);
  CHECK(Elem::news == 1 && Elem::deletes == 1);
  return 0;
}
