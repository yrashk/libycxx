// EXPECT-ERROR: error: static assertion failed[^\n]*philox_engine: 0 < r is required
// [rand.eng.philox]/6.3: "Mandates: ... 0 < r is true".
#include <random>
#include <cstdint>

int main() {
  std::philox_engine<std::uint32_t, 32, 2, 0, 0xD256D193, 0x9E3779B9> e;
  return int(e());
}
