// [support.c.headers.other]/1: <string.h> places in the global namespace each name <cstring>
// places in std, among them the C23 additions of [cstring.syn]:
//   void* memccpy(void* s1, const void* s2, int c, size_t n);
//   char* strdup(const char* s);
//   char* strndup(const char* s, size_t size);
//   void* memset_explicit(void* s, int c, size_t n);
// (ISO C 7.26.2.2 memccpy: stops after copying c, returns a pointer past it in s1 or a null
// pointer; 7.26.2.6-7 strdup/strndup: malloc'ed copies, strndup of at most size characters,
// NUL-terminated; 7.26.6.2 memset_explicit: as memset, returns s.)
// Only <string.h> and <stdlib.h> (for free) are included.
#include <string.h>
#include <stdlib.h>

#include "check.hpp"

template <class A, class B>
constexpr bool same = __is_same(A, B);

static_assert(same<decltype(::memccpy(nullptr, nullptr, 0, 0)), void*>);
static_assert(same<decltype(::strdup("")), char*>);
static_assert(same<decltype(::strndup("", 0)), char*>);
static_assert(same<decltype(::memset_explicit(nullptr, 0, 0)), void*>);

int main() {
  char dst[8] = "xxxxxxx";
  void* r = ::memccpy(dst, "ab;cd", ';', 5);
  CHECK(r == dst + 3 && dst[0] == 'a' && dst[2] == ';' && dst[3] == 'x');
  CHECK(::memccpy(dst, "abc", '!', 3) == nullptr);

  char* d = ::strdup("hello");
  CHECK(d != nullptr && ::strcmp(d, "hello") == 0);
  ::free(d);
  char* n = ::strndup("hello", 3);
  CHECK(n != nullptr && ::strcmp(n, "hel") == 0);
  ::free(n);

  char secret[4] = {1, 2, 3, 4};
  CHECK(::memset_explicit(secret, 0, sizeof secret) == secret);
  CHECK(secret[0] == 0 && secret[3] == 0);
  return 0;
}
