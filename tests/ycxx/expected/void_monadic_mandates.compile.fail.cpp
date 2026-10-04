// [expected.void.monadic]/3: and_then: "Mandates: U is a specialization of expected and
// is_same_v<typename U::error_type, E> is true."
#include <expected>

void f() {
  std::expected<void, long> e;
  (void)e.and_then([] { return std::expected<int, int>(1); });
}
