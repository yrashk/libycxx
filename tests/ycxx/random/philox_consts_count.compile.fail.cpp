// [rand.eng.philox]/6.1: "Mandates: sizeof...(consts) == n is true".
#include <random>
#include <cstdint>

int main() {
  std::philox_engine<std::uint32_t, 32, 4, 10, 0xCD9E8D57, 0x9E3779B9> e;
  return int(e());
}
