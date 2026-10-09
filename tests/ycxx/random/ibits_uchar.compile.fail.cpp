// EXPECT-ERROR: error: static assertion failed[^\n]*independent_bits_engine: UIntType must be an unsigned integer type at least as wide as short \(\[rand\.req\.genl\]/1\.7\)
// [rand.req.genl]/1.7 applied to independent_bits_engine<Engine, w, UIntType>: UIntType narrower
// than short makes the program ill-formed.
#include <random>

int main() {
  std::independent_bits_engine<std::mt19937, 8, unsigned char> e;
  return int(e());
}
