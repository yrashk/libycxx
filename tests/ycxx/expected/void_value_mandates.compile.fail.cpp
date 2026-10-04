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
