// [cstring.syn] (C++26 is based on ISO/IEC 9899:2024) declares, in namespace std,
//   void* memccpy(void* s1, const void* s2, int c, size_t n);             // freestanding
//   char* strdup(const char* s);
//   char* strndup(const char* s, size_t size);
//   void* memset_explicit(void* s, int c, size_t n);                      // freestanding
// [cstring.syn]/1: the same contents and meaning as C's <string.h> (ISO C 7.26.2.2 memccpy: copies
// up to and including the first c, returns a pointer to the character after the copy of c in s1,
// or a null pointer; 7.26.2.6/7 strdup/strndup: a malloc'ed copy (strndup: at most size characters,
// always NUL-terminated); 7.26.6.2 memset_explicit: as memset, returns s).
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::memccpy(nullptr, nullptr, 0, 0)), void*>);
static_assert(std::is_same_v<decltype(std::strdup("")), char*>);
static_assert(std::is_same_v<decltype(std::strndup("", 0)), char*>);
static_assert(std::is_same_v<decltype(std::memset_explicit(nullptr, 0, 0)), void*>);

int main() {
  char dst[16] = {};
  void* r = std::memccpy(dst, "key=value", '=', 9);
  CHECK(r == dst + 4 && std::memcmp(dst, "key=", 4) == 0 && dst[4] == 0);
  CHECK(std::memccpy(dst, "abc", 'z', 3) == nullptr && std::memcmp(dst, "abc", 3) == 0);

  char* d = std::strdup("hello");
  CHECK(d != nullptr && std::strcmp(d, "hello") == 0);
  std::free(d);
  char* n = std::strndup("hello", 3);
  CHECK(n != nullptr && std::strcmp(n, "hel") == 0);
  std::free(n);
  n = std::strndup("hi", 10);
  CHECK(n != nullptr && std::strcmp(n, "hi") == 0);
  std::free(n);

  char secret[8] = "pass";
  CHECK(std::memset_explicit(secret, 0, sizeof secret) == secret);
  for (char ch : secret) CHECK(ch == 0);
  return 0;
}
