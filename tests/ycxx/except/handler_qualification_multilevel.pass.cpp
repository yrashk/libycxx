// [except.handle]/3.3: a pointer handler matches if the exception object "can be converted to
// T by one or more of ... a qualification conversion" ([conv.qual]). For multi-level pointers,
// adding const at a level requires const at every outer level: int** converts to
// const int* const* and int* const*, but not to const int** (and not to void**).
// REQUIRES: exceptions
#include "check.hpp"

template <class H, class E>
int catches(E e) {
  try {
    try {
      throw e;
    } catch (H) {
      return 1;
    }
  } catch (...) {
    return 0;
  }
  return -1;
}

struct B {
  virtual ~B() = default;
};
struct D : B {};

int main() {
  int x = 0;
  int* px = &x;
  int** ppx = &px;
  int*** pppx = &ppx;

  CHECK(catches<int**>(ppx) == 1);
  CHECK(catches<int* const*>(ppx) == 1);
  CHECK(catches<const int* const*>(ppx) == 1);
  CHECK(catches<const volatile int* const volatile*>(ppx) == 1);
  CHECK(catches<const int**>(ppx) == 0);
  CHECK(catches<volatile int**>(ppx) == 0);
  CHECK(catches<void*>(ppx) == 1);
  CHECK(catches<const void*>(ppx) == 1);
  CHECK(catches<void**>(ppx) == 0);
  CHECK(catches<const int* const* const*>(pppx) == 1);
  CHECK(catches<int** const*>(pppx) == 1);
  CHECK(catches<int* const**>(pppx) == 0);
  CHECK(catches<const int***>(pppx) == 0);

  // value is preserved through the conversion
  try {
    throw ppx;
  } catch (const int* const* p) {
    CHECK(p == ppx && **p == 0);
  }

  // removing qualification is never allowed
  const int* cpx = &x;
  const int** cppx = &cpx;
  CHECK(catches<int**>(cppx) == 0);
  CHECK(catches<const int**>(cppx) == 1);
  CHECK(catches<const int* const*>(cppx) == 1);

  // derived-to-base is a conversion of the outermost pointer only
  D d;
  D* pd = &d;
  D** ppd = &pd;
  CHECK(catches<B**>(ppd) == 0);
  CHECK(catches<B* const*>(ppd) == 0);
  CHECK(catches<D* const*>(ppd) == 1);
  CHECK(catches<const B*>(pd) == 1);
  return 0;
}
