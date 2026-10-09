// EXPECT-ERROR: error: static assertion failed[^\n]*linear_congruential_engine: UIntType must be an unsigned integer type at least as wide as short \(\[rand\.req\.genl\]/1\.7\)
// [rand.req.genl]/1.7: UIntType must have a width of at least that of short; unsigned char is
// narrower, so the program is ill-formed.
#include <random>

int main() {
  std::linear_congruential_engine<unsigned char, 5, 3, 0> e;
  return int(e());
}
