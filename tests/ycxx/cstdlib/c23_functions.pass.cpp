// [cstdlib.syn] (C++26 is based on ISO/IEC 9899:2024) declares the C23 additions in namespace std:
//   void free_sized(void* ptr, size_t size);
//   void free_aligned_sized(void* ptr, size_t alignment, size_t size);
//   size_t memalignment(const void* p);                                   // freestanding
//   int strfromd(char* s, size_t n, const char* format, double fp);
//   int strfromf(char* s, size_t n, const char* format, float fp);
//   int strfroml(char* s, size_t n, const char* format, long double fp);
// and [c.malloc]/2-9 / [cstdlib.syn]/1 give them the C semantics (ISO C 7.24.3.4: free_sized
// with the size passed to malloc behaves as free; 7.24.3.5: free_aligned_sized for aligned_alloc;
// 7.24.3.? memalignment: "the maximum alignment satisfied by the provided address", 0 for a null
// pointer; 7.24.1.3 strfromd: as snprintf with the format restricted to %[.precision]{a,A,e,E,f,F,g,G},
// returning the number of characters that would have been written).
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::memalignment(nullptr)), std::size_t>);
static_assert(std::is_same_v<decltype(std::free_sized(nullptr, 0)), void>);
static_assert(std::is_same_v<decltype(std::free_aligned_sized(nullptr, 0, 0)), void>);
static_assert(std::is_same_v<decltype(std::strfromd(nullptr, 0, "", 0.0)), int>);
static_assert(std::is_same_v<decltype(std::strfromf(nullptr, 0, "", 0.0f)), int>);
static_assert(std::is_same_v<decltype(std::strfroml(nullptr, 0, "", 0.0L)), int>);

int main() {
  CHECK(std::memalignment(nullptr) == 0);
  alignas(256) static char buf[512];
  CHECK(std::memalignment(buf) >= 256);
  CHECK(std::memalignment(buf + 1) == 1);
  CHECK(std::memalignment(buf + 2) == 2);
  CHECK(std::memalignment(buf + 8) == 8);
  CHECK(std::memalignment(buf + 256) >= 256);

  void* p = std::malloc(40);
  CHECK(p != nullptr);
  std::free_sized(p, 40);
  std::free_sized(nullptr, 0);
  void* q = std::aligned_alloc(64, 128);
  CHECK(q != nullptr && std::memalignment(q) >= 64);
  std::free_aligned_sized(q, 64, 128);

  char s[32];
  CHECK(std::strfromd(s, sizeof s, "%.3f", 1.5) == 5 && std::strcmp(s, "1.500") == 0);
  CHECK(std::strfromf(s, sizeof s, "%g", 0.25f) == 4 && std::strcmp(s, "0.25") == 0);
  CHECK(std::strfroml(s, sizeof s, "%.2e", 1234.0L) == 8 && std::strcmp(s, "1.23e+03") == 0);
  CHECK(std::strfromd(s, 3, "%f", 1.0) == 8 && std::strcmp(s, "1.") == 0);  // truncated, NUL-terminated
  return 0;
}
