// EXPECT-ERROR: error: static assertion failed[^\n]*philox_engine: 0 < w <= numeric_limits<UIntType>::digits is required
// [rand.eng.philox]/6.4: "Mandates: ... 0 < w && w <= numeric_limits<UIntType>::digits is true".
#include <random>
#include <cstdint>

int main() {
  std::philox_engine<std::uint32_t, 33, 2, 10, 0xD256D193, 0x9E3779B9> e;
  return int(e());
}
