// EXPECT-ERROR: error: static assertion failed[^\n]*uniform_int_distribution: IntType must be a standard integer type \(\[rand\.req\.genl\]/1\.6\)
// [rand.req.genl]/1.1: "If T has a template type parameter named Sseq, URBG, Engine, RealType,
// IntType, or UIntType, the program is ill-formed if the corresponding template argument is
// cv-qualified."
#include <random>

int main() {
  std::mt19937 g;
  std::uniform_int_distribution<const int> d(0, 9);
  return d(g);
}
