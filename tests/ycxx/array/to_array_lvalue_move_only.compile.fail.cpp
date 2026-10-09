// EXPECT-ERROR: error: static assertion failed[^\n]*std::to_array: T must be copy constructible
// [array.creation]/1: to_array(T(&)[N]): "Mandates: ... is_constructible_v<remove_cv_t<T>, T&>
// is true." A move-only element type cannot be copied from an lvalue array.
#include <array>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
};

void f() {
  MoveOnly a[2];
  (void)std::to_array(a);
}
