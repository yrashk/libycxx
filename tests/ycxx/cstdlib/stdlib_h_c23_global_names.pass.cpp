// [support.c.headers.other]/1: <stdlib.h> places in the global namespace each name <cstdlib>
// places in std, including the C23 additions of [cstdlib.syn]:
//   void free_sized(void* ptr, size_t size);
//   void free_aligned_sized(void* ptr, size_t alignment, size_t size);
//   size_t memalignment(const void* p);
//   int strfromd(char* s, size_t n, const char* format, double fp);
//   int strfromf(char* s, size_t n, const char* format, float fp);
//   int strfroml(char* s, size_t n, const char* format, long double fp);
// with their C semantics (ISO C 7.24.3.4-5, 7.24.3.? memalignment: "the maximum alignment
// satisfied by the provided address", 0 for a null pointer; 7.24.1.3 strfromd: as snprintf with
// the restricted format). Only <stdlib.h> is included.
#include <stdlib.h>

#include "check.hpp"

template <class A, class B>
constexpr bool same = __is_same(A, B);

static_assert(same<decltype(::memalignment(nullptr)), size_t>);
static_assert(same<decltype(::free_sized(nullptr, 0)), void>);
static_assert(same<decltype(::free_aligned_sized(nullptr, 0, 0)), void>);
static_assert(same<decltype(::strfromd(nullptr, 0, "", 0.0)), int>);
static_assert(same<decltype(::strfromf(nullptr, 0, "", 0.0f)), int>);
static_assert(same<decltype(::strfroml(nullptr, 0, "", 0.0L)), int>);

int main() {
  CHECK(::memalignment(nullptr) == 0);
  void* p = ::malloc(24);
  CHECK(p != nullptr && ::memalignment(p) >= alignof(long double) && ::memalignment(p) % alignof(long double) == 0);
  ::free_sized(p, 24);
  void* q = ::aligned_alloc(64, 128);
  CHECK(q != nullptr && ::memalignment(q) >= 64);
  ::free_aligned_sized(q, 64, 128);
  ::free_sized(nullptr, 0);

  char buf[32];
  CHECK(::strfromd(buf, sizeof buf, "%.3f", 1.5) == 5 && buf[0] == '1' && buf[4] == '0' && buf[5] == 0);
  CHECK(::strfromf(buf, sizeof buf, "%g", 0.25f) == 4 && buf[3] == '5');
  CHECK(::strfroml(nullptr, 0, "%e", 1.0L) == 12);  // "1.000000e+00": the length that would be written
  return 0;
}
