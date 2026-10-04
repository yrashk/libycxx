// [simd.ctor]/13: Mandates: if Flags does not contain convert-flag, the conversion from
// range_value_t<R> to value_type is value-preserving. double -> float is not; the control passes
// flag_convert.
#include <simd>
#include <array>

int main() {
  std::array<double, 4> a{};
  std::simd::vec<float, 4> ok(a, std::simd::flag_convert);  // control
  (void)ok;
#ifndef YCXX_CONTROL
  std::simd::vec<float, 4> bad(a);
  (void)bad;
#endif
}
