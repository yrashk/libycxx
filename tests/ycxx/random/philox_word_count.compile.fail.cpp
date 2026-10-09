// EXPECT-ERROR: error: static assertion failed[^\n]*philox_engine: n must be 2 or 4
// [rand.eng.philox]/6.2: "Mandates: ... n == 2 || n == 4 is true".
#include <random>
#include <cstdint>

int main() {
  std::philox_engine<std::uint32_t, 32, 6, 10, 1, 2, 3, 4, 5, 6> e;
  return int(e());
}
