// EXPECT-ERROR: error: static assertion failed[^\n]*std::expected::and_then: F must return a specialization of expected with the same error_type
// [expected.void.monadic]/3: and_then: "Mandates: U is a specialization of expected and
// is_same_v<typename U::error_type, E> is true."
#include <expected>

void f() {
  std::expected<void, long> e;
  (void)e.and_then([] { return std::expected<int, int>(1); });
}
