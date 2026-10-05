// [expected.object.obs]/9: value() &: "Mandates: is_copy_constructible_v<E> is true."
// EXPECT-ERROR: static assertion failed.*std::expected::value: E must be copy constructible
#include <expected>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
};

void f() {
  std::expected<int, MoveOnly> e;
  (void)e.value();
}
