// EXPECT-ERROR: error: static assertion failed[^\n]*linear_congruential_engine: UIntType must be an unsigned integer type at least as wide as short \(\[rand\.req\.genl\]/1\.7\)
// [rand.req.genl]/1.7 permits an implementation-defined additional subset of unsigned
// integer types. On the supported targets unsigned char is narrower than short, and
// libycxx's additional UIntType subset is empty (DECISIONS.md section 10).
// This rejection tests that documented subset, not a universal draft prohibition.
#include <random>

int main() {
  std::linear_congruential_engine<unsigned char, 5, 3, 0> e;
  return int(e());
}
