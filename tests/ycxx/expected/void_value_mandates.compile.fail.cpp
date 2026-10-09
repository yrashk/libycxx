// EXPECT-ERROR: error: static assertion failed[^\n]*std::expected::value: E must be copy constructible
// [expected.void.obs]/4: value() const &: "Mandates: is_copy_constructible_v<E> is true."
#include <expected>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
};

void f() {
  std::expected<void, MoveOnly> e;
  e.value();
}
