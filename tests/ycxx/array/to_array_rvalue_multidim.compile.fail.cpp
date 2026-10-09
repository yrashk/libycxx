// EXPECT-ERROR: error: static assertion failed[^\n]*std::to_array: multidimensional arrays are not supported
// [array.creation]/4: to_array(T(&&)[N]): "Mandates: is_array_v<T> is false and
// is_constructible_v<remove_cv_t<T>, T> is true."
#include <array>
#include <utility>

void f() {
  int a[2][1] = {};
  (void)std::to_array(std::move(a));
}
