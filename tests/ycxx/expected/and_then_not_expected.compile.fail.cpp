// EXPECT-ERROR: error: static assertion failed[^\n]*std::expected::and_then: F must return a specialization of expected with the same error_type
// [expected.object.monadic]/3: "Mandates: U is a specialization of expected ..."
#include <expected>
#include <utility>

void f() {
  std::expected<int, long> e(1);
  (void)e.and_then([](int x) { return x; });
}
