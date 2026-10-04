// [simd.loadstore]/2: unchecked_load Mandates: if ranges::size(r) is a constant expression then
// ranges::size(r) >= V::size(). (partial_load has no such requirement: the control.)
#include <simd>
#include <array>

int main() {
  std::array<int, 2> a{};
  auto ok = std::simd::partial_load<std::simd::vec<int, 4>>(a);  // control
  (void)ok;
#ifndef YCXX_CONTROL
  auto bad = std::simd::unchecked_load<std::simd::vec<int, 4>>(a);
  (void)bad;
#endif
}
