// EXPECT-ERROR: error: static assertion failed[^\n]*std::unexpected: x\.error\(\) == y\.error\(\) must be well\-formed and convertible to bool
// [expected.un.eq]/1: "Mandates: The expression x.error() == y.error() is well-formed and its
// result is convertible to bool."
#include <expected>

struct NoEq {};

void f() {
  std::unexpected<NoEq> a(NoEq{}), b(NoEq{});
  (void)(a == b);
}
