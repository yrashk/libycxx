// [simd.loadstore]/17.2: partial_store (and so unchecked_store) Mandates: without convert-flag the
// conversion from T to range_value_t<R> is value-preserving. int -> short is not.
#include <simd>
#include <array>

int main() {
  std::array<short, 4> out{};
  std::simd::vec<int, 4> v(1);
  std::simd::unchecked_store(v, out, std::simd::flag_convert);  // control
#ifndef YCXX_CONTROL
  std::simd::unchecked_store(v, out);
#endif
}
