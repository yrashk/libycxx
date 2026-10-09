// EXPECT-ERROR: error: static assertion failed[^\n]*std::simd::permute: the index map gives an index out of range
// [simd.permute.static]/3: Mandates: gen-fn(i) is a constant expression whose value is
// zero_element, uninit_element, or in [0, V::size()), for all i in [0, N). Index 8 is out of
// range for vec<int, 8>; the control maps to 7.
#include <simd>

int main() {
  std::simd::vec<int, 8> v(1);
  auto ok = std::simd::permute(v, [](int) { return 7; });  // control
  (void)ok;
#ifndef YCXX_CONTROL
  auto bad = std::simd::permute(v, [](int) { return 8; });
  (void)bad;
#endif
}
