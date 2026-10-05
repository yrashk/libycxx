// [cstdarg.syn]: std::va_list and the macros va_arg, va_copy, va_end and va_start(V, ...);
// /1.2: "The preprocessing tokens comprising the second and subsequent arguments to va_start
// (if any) are discarded" (Note 1: "va_start accepts a second argument for compatibility with
// prior revisions of C++"), so va_start(ap) works, also in a function whose only parameter is
// the ellipsis, and the second argument need not name the last parameter. /1.1: the
// promotions of [expr.call]/13 apply to the variadic arguments: integral promotions and
// float to double; std::nullptr_t is converted to void*; an unscoped enumeration is promoted;
// a scoped enumeration is not.
#include <cstdarg>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<std::va_list, va_list>);

// only an ellipsis: C23-style va_start(ap)
static int sum(...) {
  std::va_list ap;
  va_start(ap);
  int n = va_arg(ap, int);
  int s = 0;
  for (int i = 0; i < n; ++i) s += va_arg(ap, int);
  va_end(ap);
  return s;
}

// one named parameter, both forms of va_start, and va_copy
static long twice(int n, ...) {
  std::va_list ap, ap2;
  va_start(ap, n);
  va_copy(ap2, ap);
  long a = 0, b = 0;
  for (int i = 0; i < n; ++i) a += va_arg(ap, long);
  for (int i = 0; i < n; ++i) b += va_arg(ap2, long);
  va_end(ap2);
  va_end(ap);
  std::va_list ap3;
  va_start(ap3);  // one argument with a named parameter too
  long c = 0;
  for (int i = 0; i < n; ++i) c += va_arg(ap3, long);
  va_end(ap3);
  std::va_list ap4;
  va_start(ap4, these tokens are discarded);
  long d = 0;
  for (int i = 0; i < n; ++i) d += va_arg(ap4, long);
  va_end(ap4);
  return a == b && b == c && c == d ? a : -1;
}

enum Unscoped : short { u7 = 7 };
enum class Scoped : int { s9 = 9 };

static bool promotions(int, ...) {
  std::va_list ap;
  va_start(ap);
  bool ok = true;
  ok = ok && va_arg(ap, int) == 'x';                  // char -> int
  ok = ok && va_arg(ap, int) == 1;                    // bool -> int
  ok = ok && va_arg(ap, int) == -3;                   // short -> int
  ok = ok && va_arg(ap, int) == 250;                  // unsigned char -> int
  ok = ok && va_arg(ap, double) == 1.5;               // float -> double
  ok = ok && va_arg(ap, void*) == nullptr;            // nullptr_t -> void*
  ok = ok && va_arg(ap, int) == 7;                    // unscoped enumeration, promoted
  ok = ok && va_arg(ap, Scoped) == Scoped::s9;        // scoped enumeration, not promoted
  ok = ok && va_arg(ap, long double) == 2.25L;
  ok = ok && va_arg(ap, const char*)[1] == 'b';
  va_end(ap);
  return ok;
}

int main() {
  CHECK(sum(3, 10, 20, 30) == 60);
  CHECK(sum(0) == 0);
  CHECK(twice(4, 1L, 2L, 3L, 4L) == 10);
  CHECK(promotions(0, 'x', true, static_cast<short>(-3), static_cast<unsigned char>(250), 1.5f, nullptr, u7,
                   Scoped::s9, 2.25L, "ab"));
}
