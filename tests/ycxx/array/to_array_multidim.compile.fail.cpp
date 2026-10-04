// [array.creation]/1: to_array: "Mandates: is_array_v<T> is false and
// is_constructible_v<remove_cv_t<T>, T&> is true."
#include <array>

void f() {
  int a[2][3] = {};
  (void)std::to_array(a);
}
