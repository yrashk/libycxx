// [uncaught.exceptions]: int uncaught_exceptions() noexcept; "Returns: The number of uncaught
// exceptions." An exception is uncaught from the throw until a handler is activated (e.g.
// while destructors run during stack unwinding).
// COUNTERPART: libstdcxx:18_support/exception_ptr/62258.cc
// COUNTERPART: libstdcxx:18_support/uncaught_exception/14026.cc
#include <exception>
#include <type_traits>
#include "check.hpp"

static_assert(noexcept(std::uncaught_exceptions()));
static_assert(std::is_same_v<decltype(std::uncaught_exceptions()), int>);

static int seen = -1;
struct Probe {
  ~Probe() { seen = std::uncaught_exceptions(); }
};

int main() {
  CHECK(std::uncaught_exceptions() == 0);
  try {
    Probe p;
    throw 1;
  } catch (int) {
    CHECK(std::uncaught_exceptions() == 0);
  }
  CHECK(seen == 1);
  { Probe p; }
  CHECK(seen == 0);
  return 0;
}
