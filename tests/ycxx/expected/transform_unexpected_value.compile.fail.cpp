// [expected.object.monadic]/19: "Mandates: U is a valid value type for expected."
// (A specialization of unexpected is not a valid value type.)
#include <expected>
#include <utility>

void f() {
  std::expected<int, long> e(1);
  (void)e.transform([](int) { return std::unexpected<int>(1); });
}
