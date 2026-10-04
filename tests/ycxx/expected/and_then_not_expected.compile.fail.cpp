// [expected.object.monadic]/3: "Mandates: U is a specialization of expected ..."
#include <expected>
#include <utility>

void f() {
  std::expected<int, long> e(1);
  (void)e.and_then([](int x) { return x; });
}
