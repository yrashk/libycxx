// [expected.object.monadic]/11: "Mandates: G is a specialization of expected and
// is_same_v<typename G::value_type, T> is true."
#include <expected>
#include <utility>

void f() {
  std::expected<int, long> e(1);
  (void)e.or_else([](long) { return std::expected<long, long>(1); });
}
