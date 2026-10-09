// EXPECT-ERROR: error: static assertion failed[^\n]*std::get: index out of range for std::array
// [array.tuple]/2: get<I>(array<T, N>&): "Mandates: I < N is true."
#include <array>

void f() {
  std::array<int, 2> a{};
  (void)std::get<2>(a);
}
