// [expected.object.monadic]/27: "Mandates: G is a valid template argument for unexpected"
// (void is not an object type).
#include <expected>
#include <utility>

void f() {
  std::expected<int, long> e(1);
  (void)e.transform_error([](long) {});
}
