// EXPECT-ERROR: error: static assertion failed[^\n]*std::simd: the conversion from the range's value type to V::value_type is not value\-preserving \(pass flag_convert\)
// [simd.loadstore]/7.4: partial_load (and so unchecked_load) Mandates: without convert-flag the
// conversion from range_value_t<R> to V::value_type is value-preserving. int -> short is not.
#include <simd>
#include <array>

int main() {
  std::array<int, 4> a{};
  auto ok = std::simd::unchecked_load<std::simd::vec<short, 4>>(a, std::simd::flag_convert);  // control
  (void)ok;
#ifndef YCXX_CONTROL
  auto bad = std::simd::unchecked_load<std::simd::vec<short, 4>>(a);
  (void)bad;
#endif
}
