// EXPECT-ERROR: error: static assertion failed[^\n]*independent_bits_engine: UIntType must be an unsigned integer type at least as wide as short \(\[rand\.req\.genl\]/1\.7\)
// [rand.req.genl]/1.7 permits an implementation-defined additional subset of unsigned
// integer types, including narrow types for independent_bits_engine. On supported targets
// unsigned char is narrower than short; libycxx's additional UIntType subset is empty
// (DECISIONS.md section 10). This is a supported-type policy test, not universal rejection.
#include <random>

int main() {
  std::independent_bits_engine<std::mt19937, 8, unsigned char> e;
  return int(e());
}
