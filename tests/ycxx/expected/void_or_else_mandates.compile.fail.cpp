// [expected.void.monadic]/10: or_else: "Mandates: G is a specialization of expected and
// is_same_v<typename G::value_type, T> is true."
#include <expected>

void f() {
  std::expected<void, long> e;
  (void)e.or_else([](long) { return std::expected<int, long>(1); });
}
