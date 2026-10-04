// [expected.object.obs]/12: value() &&: "Mandates: is_copy_constructible_v<E> is true and
// is_constructible_v<E, decltype(std::move(error()))> is true."
// A move-only E is not copy-constructible, so even the rvalue overload is ill-formed.
#include <expected>
#include <utility>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
};

void f() {
  std::expected<int, MoveOnly> e;
  (void)std::move(e).value();
}
