// [expected.object.obs]/23: error_or: "Mandates: is_copy_constructible_v<E> is true and
// is_convertible_v<G, E> is true."
#include <expected>

struct Ex { explicit Ex(int) {} };

void f() {
  std::expected<int, Ex> e;
  (void)e.error_or(1);
}
